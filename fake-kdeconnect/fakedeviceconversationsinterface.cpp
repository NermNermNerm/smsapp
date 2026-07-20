// clazy:excludeall=range-loop-detach
#include "fakedeviceconversationsinterface.h"

#include <QVariantMap>
#include <QDBusVariant>
#include <QTimer>
#include <algorithm>
#include "deviceconfig.h"
#include "kdeconnect_interfaces/conversationmessage_ext.h"
#include <QDir>
#include <QList>
#include "commands.h"


static QDBusVariant toDBus(const ConversationMessage &msg)
{
    QDBusArgument arg;
    arg << msg;
    return QDBusVariant(QVariant::fromValue(arg));
}

/*
 * Constructor
 */
FakeDeviceConversationsInterface::FakeDeviceConversationsInterface(
    DeviceConfig *deviceConfig,
    QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_deviceConfig(deviceConfig)
    , m_enumerationTimer(new QTimer(this))
{
    m_enumerationTimer->setInterval(m_intervalMs);   // simulate real daemon burst timing
    connect(m_enumerationTimer, &QTimer::timeout,
            this, &FakeDeviceConversationsInterface::emitNextThread);

    m_incomingTimer = new QTimer(this);
    m_incomingTimer->setInterval(m_sendIntervalMs);
    connect(m_incomingTimer, &QTimer::timeout,
            this, &FakeDeviceConversationsInterface::processIncomingQueue);
    m_incomingTimer->start();
}

void FakeDeviceConversationsInterface::setInterval(int mSec)
{
    m_intervalMs = mSec;
    m_enumerationTimer->setInterval(m_intervalMs);
}

void FakeDeviceConversationsInterface::setSendInterval(int mSec)
{
    m_sendIntervalMs = mSec;
    m_incomingTimer->setInterval(m_sendIntervalMs);
}

/**
 *  Called (indirectly) by the test SMS client to say "stop any activity started by previous
 *  clients"
 */
void FakeDeviceConversationsInterface::reset()
{
    QMutexLocker locker(&m_enumerationMutex);

    m_enumerationTimer->stop();
    m_enumerationHeads.clear();
    m_nextIndex = 0;
}

/*
 * Helper: visibly truncate metadata-only messages
 */
ConversationMessage FakeDeviceConversationsInterface::metadataTruncate(
    const ConversationMessage &msg)
{
    QString mungedBody = msg.body();
    int msgLength = mungedBody.size();

    if (msgLength >= 3 && msgLength <= 10) {
        mungedBody = mungedBody.left(msgLength - 2) + "]";
    } else if (msgLength > 10) {
        mungedBody = "[" + mungedBody.left(qMin(msgLength, 43) - 3) + "]";
    }

    return ConversationMessage(
        msg.eventField(),
        mungedBody,
        msg.addresses(),
        msg.date(),
        msg.type(),
        msg.read(),
        msg.threadID(),
        msg.uID(),
        msg.subID(),
        msg.attachments()
        );
}

/*
 * requestAllConversationThreads()
 *
 * Real daemon behavior:
 * - If device unreachable → silent no-op
 * - Otherwise:
 *     - Build thread heads (oldest message per thread)
 *     - Mung metadata-only messages
 *     - Sort by date
 *     - Emit one thread every 50ms
 */
void FakeDeviceConversationsInterface::requestAllConversationThreads()
{
    if (!m_deviceConfig->reachable) {
        qInfo() << "requestAllConversationThreads invoked, but the device is offline.  Ignoring.";
        return; // silent no-op like real daemon
    }

    if (m_enumerationTimer->isActive()) {
        qWarning() << "requestAllConversationThreads() called while enumeration is ongoing."
                   << "Client is misbehaving; ignoring request.";
        return;
    }

    m_enumerationTimer->stop();
    m_enumerationHeads.clear();
    m_nextIndex = 0;

    // Build map: threadId → newest message
    QHash<qint64, ConversationMessage> headMap;

    for (const auto &msg : m_deviceConfig->smsMessages) {
        auto it = headMap.find(msg.threadID());
        if (it == headMap.end()) {
            headMap.insert(msg.threadID(), msg);
        } else {
            // newest message becomes the head
            if (msg.date() > it->date())
                it.value() = msg;
        }
    }

    // Convert to vector and mung metadata-only messages
    m_enumerationHeads.reserve(headMap.size());
    for (auto it = headMap.begin(); it != headMap.end(); ++it) {
        ConversationMessage head = it.value();

        if (!head.containsAttachment())
            head = metadataTruncate(head);

        m_enumerationHeads.push_back(head);
    }

    // Sort by head date
    std::sort(m_enumerationHeads.begin(), m_enumerationHeads.end(),
              [](const ConversationMessage &a, const ConversationMessage &b) {
                  return a.date() > b.date();
              });

    qInfo() << "requestAllConversationThreads invoked; sending" << m_enumerationHeads.size() << "conversation headers";
    m_nextIndex = 0;
    m_enumerationTimer->start();
}


void FakeDeviceConversationsInterface::replyToConversation(
    qlonglong conversationID,
    const QString &message,
    const QVariantList &attachmentUrls) // <- actually local paths!
{
    qInfo() << "replyToConversation invoked conversationID:" << conversationID << "message: " << message.left(20);

    auto conversationMessage = m_deviceConfig->makeMessage(conversationID, message, DeviceConfig::Outgoing, attachmentUrls);
    if (conversationMessage.threadID() != conversationID || conversationID == 0) {
        // ^^ assumes that conversationID of 0 is actually invalid
        // vv no warning -- makeMessage did that already
        return;
    }

    simulateIncomingMessage(conversationMessage);
}

void FakeDeviceConversationsInterface::sendWithoutConversation(
    const QVariantList &addressList,
    const QString &message,
    const QVariantList &attachmentUrls)
{
    Q_UNUSED(attachmentUrls)

    QStringList addresses;
    addresses.reserve(addressList.size());

    for (const QVariant &v : addressList) {
        if (!v.canConvert<QString>()) {
            qWarning() << "Invalid address entry in sendWithoutConversation:" << v;
            return;    // or continue, depending on your error policy
        }

        const QString s = v.toString().trimmed();
        if (s.isEmpty()) {
            qWarning() << "Empty address entry in sendWithoutConversation";
            return;
        }

        addresses.append(s);
    }

    auto conversationMessage = m_deviceConfig->makeMessage(addresses, message, DeviceConfig::Outgoing, attachmentUrls);
    simulateIncomingMessage(conversationMessage);
}

void FakeDeviceConversationsInterface::requestConversation(
    qlonglong conversationID,
    int start,
    int end)
{
    qInfo() << "requestConversation" << conversationID << start << end;
    // Gather matching messages
    std::vector<ConversationMessage> result;
    for (const auto &msg : m_deviceConfig->smsMessages) {
        if (msg.threadID() == conversationID)
            result.push_back(msg);
    }

    // Sort newest-first
    std::sort(result.begin(), result.end(),
              [&](const ConversationMessage &a, const ConversationMessage &b) {
                  return isNewerMessage(a, b);
              });

    // Trim to start..end inclusive
    if (start < 0) start = 0;
    if (end >= static_cast<int>(result.size()))
        end = result.size() - 1;

    if (start > end)
        return;

    result.erase(result.begin() + end + 1, result.end());
    result.erase(result.begin(), result.begin() + start);

    // Nothing to send
    if (result.empty()) {
        qInfo() << "requestConversation call resulted in no results; silently doing nothing";
        return;
    }

    // Create a tiny ephemeral job object
    QObject *job = new QObject(this);

    // Store state inside the job using lambdas
    auto messages = std::make_shared<std::vector<ConversationMessage>>(std::move(result));
    auto index    = std::make_shared<int>(0);

    // Create a timer owned by the job
    QTimer *timer = new QTimer(job);
    timer->setInterval(m_intervalMs);
    timer->setSingleShot(false);

    // Timer callback
    connect(timer, &QTimer::timeout, this,
            [this, job, timer, messages, index]() {

                if (*index >= static_cast<int>(messages->size())) {
                    timer->stop();
                    job->deleteLater();   // self-destruct
                    return;
                }

                const ConversationMessage &msg = (*messages)[(*index)++];
                qInfo() << "emit ConversationUpdated" << msg.threadID() << msg.body().left(20);
                emit conversationUpdated(toDBus(msg));
            });

    timer->start();
}

void FakeDeviceConversationsInterface::requestAttachmentFile(qlonglong partID, const QString &uniqueIdentifier)
{
    qInfo() << "requestAttachmentFile" << partID << uniqueIdentifier;
    QString filePath = QDir::homePath() + "/fakekde/" + m_deviceConfig->id + "/" + uniqueIdentifier;
    if (!QFile::exists(filePath)) {
        qWarning() << "requestAttachmentFile" << partID << uniqueIdentifier << "Attachment file was not found!";
        return;
    }

    QTimer *timer = new QTimer(this);
    timer->setInterval(m_attachmentIntervalMs);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, [this, filePath, uniqueIdentifier, timer]() {
        qInfo() << "emit attachmentReceived" << filePath << uniqueIdentifier;
        emit attachmentReceived(filePath, uniqueIdentifier);
        timer->deleteLater();
    });
    timer->start();
}

/*
 * Called on timer ticks to urp out conversationCreated/loaded signals
 *
 * Emits:
 *   conversationCreated(headMessage)  <- sometimes
 *   conversationLoaded(threadId, 1)
 *
 * One thread per timer tick.
 */
void FakeDeviceConversationsInterface::emitNextThread()
{
    QMutexLocker locker(&m_enumerationMutex);

    if (m_nextIndex >= m_enumerationHeads.size()) {
        m_enumerationTimer->stop();
        qInfo() << "requestAllConversationThreads enumeration complete";
        return;
    }

    const ConversationMessage &head = m_enumerationHeads[m_nextIndex++];
    const qint64 threadId = head.threadID();


    // Emit conversationCreated only once per thread per "session"
    if (!m_knownThreads.contains(threadId)) {
        qInfo() << "emit conversationCreated" << threadId << head.body().left(20);
        emit conversationCreated(toDBus(head));
        m_knownThreads.insert(threadId);
    }

    // Count is intentionally nonsense; client ignores it
    qInfo() << "emit conversationLoaded" << threadId;
    emit conversationLoaded(threadId, 9765);
}

void FakeDeviceConversationsInterface::simulateIncomingMessage(const ConversationMessage &m)
{
    if (m.body().contains("TESTBREAK")) {
        qInfo() << "body contains TESTBREAK, dumping the message on the floor";
        return;
    }

    QMutexLocker locker(&m_incomingMutex);
    m_incomingQueue.enqueue(m);
    m_deviceConfig->smsMessages.append(m);
    m_deviceConfig->save();
}

void FakeDeviceConversationsInterface::processIncomingQueue()
{
    if (!m_deviceConfig->reachable)
        return;

    QMutexLocker locker(&m_incomingMutex);
    if (m_incomingQueue.isEmpty())
        return;

    ConversationMessage m = m_incomingQueue.dequeue();
    locker.unlock();

    deliverIncomingMessageNow(m);
}

void FakeDeviceConversationsInterface::deliverIncomingMessageNow(const ConversationMessage &m)
{
    const qint64 threadId = m.threadID();

    if (!m_knownThreads.contains(threadId)) {
        qInfo() << "emit ConversationCreated" << threadId << m.body().left(20);
        emit conversationCreated(toDBus(m));
        m_knownThreads.insert(threadId);
    }
    else {
        qInfo() << "emit ConversationUpdated" << threadId << m.body().left(20);
        emit conversationUpdated(toDBus(m));
    }
    qInfo() << "emit ConversationLoaded" << threadId;
    emit conversationLoaded(threadId, nonsenseCountValue);
}
