#include "harvester.h"
#include "dbus.h"
#include "kdeconnect_interfaces/kdeconnect_proxy.h"
#include <iostream>

#include <QCoreApplication>
#include <QDBusPendingReply>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QQueue>
#include <QTimer>
#include <QDebug>
#include <QEventLoop>
#include <QDBusVariant>
#include <QMutex>
#include <QMutexLocker>
#include <functional>



namespace harvester {

QVector<QString> readAllDeviceIds()
{
    auto &daemon = dbus::daemon();
    QDBusPendingReply<QStringList> reply = daemon.devices(/* onlyReachable */ false, /* onlyPaired = */ true);
    reply.waitForFinished();
    QVector<QString> result;
    if (reply.isError())
    {
        qWarning() << "daemon.devices failed:" << reply.value();
        return result;
    }

    for (const QString &id : reply.value()) {
        auto &dev = dbus::device(id);
        if (dev.supportedPlugins().contains("kdeconnect_sms"))
        {
            result.push_back(id);
        }
    }

    return result;
}

QVector<qint64> readAllThreadIds(const QString &deviceId)
{
    QVector<qint64> result;

    auto &conversations = dbus::conversations(deviceId);

    QEventLoop loop;

    QTimer tickTimer;
    tickTimer.setInterval(1000);   // 1 Hz
    tickTimer.setSingleShot(false);

    QElapsedTimer sinceLastMessage;
    sinceLastMessage.invalidate();

    qint64 lastThreadId = -1;

    std::string lastStatusLine;

    auto printStatus = [&](const char *reason) {
        std::ostringstream oss;
        oss << "Reading threads: "
            << result.size()
            << " received (last=" << lastThreadId
            << ", reason=" << reason << ")";

        std::string line = oss.str();

        // Pad with spaces if shorter than previous
        if (line.size() < lastStatusLine.size()) {
            line.append(lastStatusLine.size() - line.size(), ' ');
        }

        std::cout << line << '\r' << std::flush;
        lastStatusLine = oss.str();
    };

    // When a thread arrives
    QObject::connect(
        &conversations,
        &org::kde::kdeconnect::conversations::conversationLoaded,
        &loop,
        [&](qint64 threadId, int /*count*/) {
            result.push_back(threadId);
            lastThreadId = threadId;

            if (!sinceLastMessage.isValid()) {
                sinceLastMessage.start();
            } else {
                sinceLastMessage.restart();
            }

            printStatus("new");
        }
        );

    // Periodic timer checks halt condition
    QObject::connect(&tickTimer, &QTimer::timeout, &loop, [&]() {
        if (!sinceLastMessage.isValid()) {
            // No messages yet — keep waiting
            return;
        }

        qint64 elapsed = sinceLastMessage.elapsed();
        if (lastThreadId < 10 && elapsed > 10000) {
            printStatus("halt");
            loop.quit();
            return;
        }

        printStatus("tick");
    });

    printStatus("start");
    QDBusPendingReply<void> callReply = conversations.requestAllConversationThreads();
    callReply.waitForFinished();

    if (callReply.isError()) {
        qWarning() << "DBus error calling requestAllConversationThreads: "
                   << callReply.error().message().toStdString();
    }

    tickTimer.start();

    loop.exec();

    return result;
}

class ConversationReader
{
public:
    explicit ConversationReader(const QString &deviceId, const QString &fakeDeviceId)
        : m_deviceId(deviceId),
        m_loop(),
        m_outDir(QDir::homePath() + "/fakekde/" + fakeDeviceId)
    {
        if (!m_outDir.exists() && !m_outDir.mkpath(".")) {
            qWarning() << "ConversationReader: failed to create output dir" << m_outDir.path();
        }
    }

    QVector<ConversationMessage> run(qint64 threadId)
    {
        {
            QMutexLocker locker(&m_mutex);
            m_result.clear();
            m_queue.clear();
        }
        m_threadId = threadId;

        auto &conversations = dbus::conversations(m_deviceId);

        // Timer and status helpers
        QTimer tickTimer;
        tickTimer.setInterval(1000);
        tickTimer.setSingleShot(false);

        m_sinceLastMessage.invalidate();
        m_lastStatusLine.clear();

        // Connect signals -> member methods using m_loop as context object
        // conversationUpdated(QDBusVariant)
        QObject::connect(&conversations,
                         &org::kde::kdeconnect::conversations::conversationUpdated,
                         &m_loop,
                         std::bind(&ConversationReader::onConversationUpdated, this, std::placeholders::_1),
                         Qt::QueuedConnection);

        // attachmentReceived(QString filePath, QString fileName)
        QObject::connect(&conversations,
                         &org::kde::kdeconnect::conversations::attachmentReceived,
                         &m_loop,
                         std::bind(&ConversationReader::onAttachmentReceived, this, std::placeholders::_1, std::placeholders::_2),
                         Qt::QueuedConnection);

        // Tick timer -> onTick
        QObject::connect(&tickTimer, &QTimer::timeout, &m_loop,
                         std::bind(&ConversationReader::onTick, this),
                         Qt::QueuedConnection);

        qInfo() << "Retrieving thread" << threadId;

        // Kick off the request for conversation history
        QDBusPendingReply<void> callReply = conversations.requestConversation(threadId, 0, 9999);
        callReply.waitForFinished();
        if (callReply.isError()) {
            std::cout << "DBus error calling requestConversation: "
                      << callReply.error().message().toStdString()
                      << std::endl << std::endl;
            return m_result;
        }

        tickTimer.start();
        m_loop.exec();
        std::cout << " \r" << std::flush; // clean up status line
        return m_result;
    }

private:

    // ---- signal handlers and helpers (methods) ----

    void onConversationUpdated(const QDBusVariant &variant)
    {
        ConversationMessage msg = ConversationMessage::fromDBus(variant);
        if (msg.threadID() != m_threadId) return;

        bool wasEmpty = false;
        bool isStillEmpty = true;
        {
            QMutexLocker locker(&m_mutex);
            wasEmpty = m_queue.isEmpty();
            for (const Attachment &a : msg.attachments()) {
                QString destPath = m_outDir.filePath(a.uniqueIdentifier());
                if (!QFile::exists(destPath)) {
                    m_queue.enqueue(a);
                }
            }
            isStillEmpty = m_queue.isEmpty();
        }

        // If queue was empty before we added attachments, request the head now.
        if (wasEmpty && !isStillEmpty) {
            // schedule on next event loop turn to keep ordering consistent
            QTimer::singleShot(0, QCoreApplication::instance(), [this]() { requestNextIfNeeded(); });
        }

        if (!m_sinceLastMessage.isValid()) m_sinceLastMessage.start(); else m_sinceLastMessage.restart();
        sayImAlive();
    }

    void onAttachmentReceived(const QString &filePath, const QString &fileName)
    {
        bool isQueueEmpty;
        Attachment oldHead;
        {
            QMutexLocker locker(&m_mutex);
            oldHead = m_queue.head();
            m_queue.removeFirst();
            isQueueEmpty = m_queue.isEmpty();
        }

        Q_ASSERT(oldHead.uniqueIdentifier() == fileName);
        // Copy (overwrite) into outDir
        QString dest = m_outDir.filePath(oldHead.uniqueIdentifier());
        if (QFile::exists(dest)) {
            if (!QFile::remove(dest)) {
                qWarning() << "ConversationReader: failed to remove existing file" << dest;
            }
        }
        if (!QFile::copy(filePath, dest)) {
            qWarning() << "ConversationReader: failed to copy" << filePath << "->" << dest;
        } else {
            sayImAlive();
        }

        if (!isQueueEmpty)
            QTimer::singleShot(0, QCoreApplication::instance(), [this]() { requestNextIfNeeded(); });
    }

    void onTick()
    {
        if (!m_sinceLastMessage.isValid()) {
            return;
        }
        qint64 elapsed = m_sinceLastMessage.elapsed();

        bool isAttachmentQueueEmpty;
        {
            QMutexLocker locker(&m_mutex);
            isAttachmentQueueEmpty = m_queue.isEmpty();
        }

        if (elapsed > 10000 && isAttachmentQueueEmpty) {
            m_loop.quit();
            return;
        }
    }

    void requestNextIfNeeded()
    {
        Attachment head;
        {
            QMutexLocker locker(&m_mutex);
            Q_ASSERT(!m_queue.isEmpty());
            head = m_queue.head();
        }

        auto &conversations = dbus::conversations(m_deviceId);
        QDBusPendingReply<void> reply = conversations.requestAttachmentFile(head.partID(), head.uniqueIdentifier());
        reply.waitForFinished();
        if (reply.isError()) {
            qFatal() << "ConversationReader: requestAttachmentFile immediate error for part"
                       << head.partID() << ":" << reply.error().message();
            // Note this leaves the head on the queue, meaning that it will hang here indefinitely.
            //  Given that this sort of failure spells the doom of the whole undertaking (because the
            //  kde daemon crashed or something), there's no reason to carry on anyway.
        }
    }

    // ---- small helpers ----
    void sayImAlive()
    {
        std::string line;
        if (m_lastStatusLine == "" || m_lastStatusLine == "|")
            line = "/";
        else if (m_lastStatusLine == "/")
            line = "-";
        else if (m_lastStatusLine == "-")
            line = "\\";
        else // line == "\\"
            line = "|";

        std::cout << line << '\r' << std::flush;
        m_lastStatusLine = line;
    }

private:
    QString m_deviceId;
    QEventLoop m_loop;
    QDir m_outDir;

    QVector<ConversationMessage> m_result;

    QQueue<Attachment> m_queue;
    qint64 m_threadId = -1;

    QElapsedTimer m_sinceLastMessage;
    std::string m_lastStatusLine;

    QMutex m_mutex;
};

QVector<ConversationMessage> readAllMessages(const QString &deviceId, qint64 threadId, const QString &fakeDeviceId)
{
    ConversationReader reader(deviceId, fakeDeviceId);
    return reader.run(threadId);
}

}
