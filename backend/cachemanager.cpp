#include "cachemanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>

#include "kdeconnect_interfaces/conversationmessage_ext.h"

CacheManager::CacheManager(QObject *parent)
    : QObject(parent)
{
    // Periodic save timer (every 10 seconds)
    m_timer = new QTimer(this);
    m_timer->setInterval(10'000);
    connect(m_timer, &QTimer::timeout, this, &CacheManager::saveIfDirty);
    m_timer->start();
}

CacheManager::~CacheManager()
{
    if (m_timer) {
        m_timer->stop();
    }
    // No I/O here on purpose; caller should have invoked saveNow() on shutdown.
}

void CacheManager::load(const QString &deviceId)
{
    Q_ASSERT(m_cachePath.isNull());

    // Decide cache path once
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!base.isEmpty()) {
        QDir().mkpath(base);
        m_cachePath = base + QLatin1String("/cache-") + deviceId + QLatin1String(".json");
    }

    if (m_cachePath.isEmpty())
        return;

    QFile file(m_cachePath);
    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "CacheManager::load: failed to open" << m_cachePath
                   << "for reading";
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "CacheManager::load: invalid JSON in" << m_cachePath
                   << "-" << err.errorString();
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonArray threadsArray = root.value(QLatin1String("threads")).toArray();

    QWriteLocker locker(&m_lock);
    m_threads.clear();

    for (const QJsonValue &tv : threadsArray) {
        const QJsonObject to = tv.toObject();

        const qint64 threadID =
            to.value(QLatin1String("threadID")).toVariant().toLongLong();
        ThreadData td;
        td.messageCount = to.value(QLatin1String("messageCount")).toInt();

        const QJsonArray msgs = to.value(QLatin1String("messages")).toArray();
        for (const QJsonValue &mv : msgs) {
            const QJsonObject mo = mv.toObject();
            const int index = mo.value(QLatin1String("index")).toInt();
            const QJsonObject msgObj = mo.value(QLatin1String("message")).toObject();
            const ConversationMessage msg = messageFromJson(msgObj);
            td.messages.insert(index, msg);
        }

        m_threads.insert(threadID, td);
    }

    m_dirty = false;
}

void CacheManager::saveNow()
{
    if (m_cachePath.isEmpty())
        return;

    QReadLocker locker(&m_lock);
    if (!saveUnlocked()) {
        qWarning() << "CacheManager::saveNow: failed to save cache to"
                   << m_cachePath;
        return;
    }

    m_dirty = false;
}

void CacheManager::saveIfDirty()
{
    if (m_cachePath.isEmpty())
        return;

    // Fast path: avoid locking if not dirty
    if (!m_dirty)
        return;

    QReadLocker locker(&m_lock);
    if (!m_dirty)
        return;

    if (!saveUnlocked()) {
        qWarning() << "CacheManager::saveIfDirty: failed to save cache to"
                   << m_cachePath;
        return;
    }

    m_dirty = false;
}

bool CacheManager::saveUnlocked() const
{
    if (m_cachePath.isEmpty())
        return false;

    QJsonArray threadsArray;

    for (auto it = m_threads.constBegin(); it != m_threads.constEnd(); ++it) {
        const qint64 threadID = it.key();
        const ThreadData &td = it.value();

        QJsonObject to;
        to.insert(QLatin1String("threadID"),
                  static_cast<qint64>(threadID));
        to.insert(QLatin1String("messageCount"),
                  td.messageCount);

        QJsonArray msgs;
        for (auto mit = td.messages.constBegin();
             mit != td.messages.constEnd(); ++mit) {
            const int index = mit.key();
            const ConversationMessage &msg = mit.value();

            QJsonObject mo;
            mo.insert(QLatin1String("index"), index);
            mo.insert(QLatin1String("message"), toJson(msg));
            msgs.append(mo);
        }

        to.insert(QLatin1String("messages"), msgs);
        threadsArray.append(to);
    }

    QJsonObject root;
    root.insert(QLatin1String("threads"), threadsArray);

    const QJsonDocument doc(root);
    const QByteArray payload = doc.toJson(QJsonDocument::Compact);

    // Atomic write: write to temp, then rename
    const QString tmpPath = m_cachePath + QLatin1String("~");
    QSaveFile tmp(tmpPath);
    if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "CacheManager::saveUnlocked: failed to open temp file"
                   << tmpPath << "for writing";
        return false;
    }

    if (tmp.write(payload) != payload.size()) {
        qWarning() << "CacheManager::saveUnlocked: failed to write full payload to"
                   << tmpPath;
        return false;
    }

    if (!tmp.commit()) {
        qWarning() << "CacheManager::saveUnlocked: commit failed for" << tmpPath;
        return false;
    }

    QFile::remove(m_cachePath);
    if (!QFile::rename(tmpPath, m_cachePath)) {
        qWarning() << "CacheManager::saveUnlocked: rename failed from"
                   << tmpPath << "to" << m_cachePath;
        return false;
    }

    return true;
}

void CacheManager::updateMessageCount(qint64 threadID, int daemonCount)
{
    QWriteLocker locker(&m_lock);

    // This *is* allowed to implicitly create a thread.
    ThreadData &td = m_threads[threadID];

    const int cachedCount = td.messageCount;

    if (daemonCount > cachedCount) {
        const int delta = daemonCount - cachedCount;

        // Shift all existing indices upward by delta
        QMap<int, ConversationMessage> shifted;
        for (auto it = td.messages.constBegin(); it != td.messages.constEnd(); ++it) {
            shifted.insert(it.key() + delta, it.value());
        }

        td.messages = std::move(shifted);
    }

    td.messageCount = daemonCount;
    m_dirty = true;
}

bool CacheManager::tryGetMessage(qint64 threadID, int index,
                                 ConversationMessage &out) const
{
    QReadLocker locker(&m_lock);

    auto it = m_threads.constFind(threadID);
    if (it == m_threads.constEnd())
        return false;

    const ThreadData &td = it.value();
    if (index < 0 || index >= td.messageCount)
        return false;

    auto mit = td.messages.constFind(index);
    if (mit == td.messages.constEnd())
        return false;

    out = mit.value();
    return true;
}

void CacheManager::storeMessage(qint64 threadID, int index,
                                const ConversationMessage &msg)
{
    QWriteLocker locker(&m_lock);
    auto it = m_threads.find(threadID);
    Q_ASSERT(it != m_threads.end());
    if (it == m_threads.end())
        return;

    ThreadData &td = it.value();
    td.messages.insert(index, msg);

    if (index + 1 > td.messageCount)
        td.messageCount = index + 1;

    m_dirty = true;
}

void CacheManager::clearThread(qint64 threadID)
{
    QWriteLocker locker(&m_lock);
    m_threads.remove(threadID);
    m_dirty = true;
}

bool CacheManager::hasMessage(qint64 threadID,
                              const ConversationMessage &msg) const
{
    QReadLocker locker(&m_lock);

    auto it = m_threads.constFind(threadID);
    if (it == m_threads.constEnd())
        return false;

    const ThreadData &td = it.value();
    for (auto mit = td.messages.constBegin();
         mit != td.messages.constEnd(); ++mit) {
        if (isSameMessage(mit.value(), msg))
            return true;
    }

    return false;
}

bool CacheManager::isNewerThanCached(const ConversationMessage &msg) const
{
    QReadLocker locker(&m_lock);

    auto it = m_threads.constFind(msg.threadID());
    if (it == m_threads.constEnd())
        return true; // no cache for this thread yet

    const ThreadData &td = it.value();
    if (td.messages.isEmpty())
        return true;

    auto mit = td.messages.constEnd();
    --mit; // last (highest index) message
    const ConversationMessage &newest = mit.value();

    return isNewerMessage(msg, newest);
}

bool CacheManager::tryGetMessageCount(qint64 threadID, int &messageCount) const
{
    auto it = m_threads.constFind(threadID);
    if (it == m_threads.constEnd()) {
        messageCount = 0;
        return false;
    } else {
        messageCount = it->messageCount;
        return true;
    }
}
