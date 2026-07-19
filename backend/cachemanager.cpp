#include "cachemanager.h"
#include "kdeconnect_interfaces/conversationmessage_ext.h"

CacheManager::CacheManager(const QString &deviceId, QObject *parent)
    : QObject(parent), m_timer(this)
{
    // Periodic save timer (every 10 seconds)
    m_timer.setInterval(10'000);
    connect(&m_timer, &QTimer::timeout, this, &CacheManager::save);
    doLoad(deviceId);
    m_timer.start();
}

void CacheManager::doLoad(const QString &deviceId)
{
    Q_ASSERT(m_cachePath.isEmpty());
    // Build cache path
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        qWarning() << "CacheManager::load: no application data location is available?!";
        return;
    }

    QDir().mkpath(base);
    m_cachePath = base + "/cache-" + deviceId + ".json";

    if (m_cachePath.isEmpty())
        return;

    QFile file(m_cachePath);
    if (!file.exists())
        return;

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "CacheManager::load: failed to open" << m_cachePath;
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

    const QJsonArray arr = doc.object().value("messages").toArray();

    m_threads.clear();

    for (const QJsonValue &v : arr) {
        const QJsonObject obj = v.toObject();
        ConversationMessage msg = messageFromJson(obj);
        m_threads[msg.threadID()].append(msg);
    }

    m_dirty = false;
}


void CacheManager::save()
{
    if (m_cachePath.isEmpty())
        return;

    if (!m_dirty)
        return;

    QReadLocker locker(&m_lock);
    if (!m_dirty)
        return;

    if (!doSave()) {
        qWarning() << "CacheManager::saveIfDirty: failed to save cache to"
                   << m_cachePath;
        return;
    }

    m_dirty = false;
}

bool CacheManager::doSave() const
{
    QJsonArray arr;

    // Flatten all messages
    for (auto it = m_threads.constBegin(); it != m_threads.constEnd(); ++it) {
        const QVector<ConversationMessage> &vec = it.value();
        for (const ConversationMessage &msg : vec)
            arr.append(toJson(msg));
    }

    QJsonObject root;
    root.insert("messages", arr);

#ifdef QT_DEBUG
    // Pretty JSON for debugging
    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
#else
    // Compact JSON for production
    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);
#endif

    // Atomic write
    const QString tmpPath = m_cachePath + "~";
    QSaveFile tmp(tmpPath);
    if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "CacheManager::saveUnlocked: failed to open temp file" << tmpPath;
        return false;
    }

    if (tmp.write(payload) != payload.size()) {
        qWarning() << "CacheManager::saveUnlocked: failed to write full payload";
        return false;
    }

    if (!tmp.commit()) {
        qWarning() << "CacheManager::saveUnlocked: commit failed";
        return false;
    }

    QFile::remove(m_cachePath);
    if (!QFile::rename(tmpPath, m_cachePath)) {
        qWarning() << "CacheManager::saveUnlocked: rename failed";
        return false;
    }

    return true;
}

void CacheManager::deleteConversation(qint64 conversationID)
{
    QWriteLocker locker(&m_lock);
    m_threads.remove(conversationID);
    m_dirty = true;
}

QVector<ConversationMessage> CacheManager::getAllConversationMessages() const
{
    QReadLocker locker(&m_lock);
    QVector<ConversationMessage> out;

    // Compute total size once
    qsizetype total = 0;
    for (const auto &vec : m_threads)
        total += vec.size();

    out.reserve(total);

    // Append all messages in map order
    for (const auto &vec : m_threads)
        out += vec;   // QVector::operator+= appends all elements

    return out;
}

QVector<ConversationMessage> CacheManager::getConversationMessages(qint64 conversationID) const
{
    QReadLocker locker(&m_lock);
    return m_threads.value(conversationID);
}

bool CacheManager::isCachedThreadOlderThan(qint64 conversationID, qint64 cutoffTime) const
{
    QReadLocker locker(&m_lock);

    const QVector<ConversationMessage> &messages = m_threads.value(conversationID);
    if (messages.isEmpty())
        return false;

    for (const ConversationMessage &msg : messages) {
        if (msg.date() >= cutoffTime)
            return false;
    }

    return true;
}

CacheManager::StoreResult CacheManager::storeMessage(const ConversationMessage &msg)
{
    QWriteLocker locker(&m_lock);

    QVector<ConversationMessage> &messages = m_threads[msg.threadID()];

    auto it = std::find_if(messages.begin(), messages.end(),
                           [&](const ConversationMessage &existing) {
                               return isSameMessageID(existing, msg);
                           });

    if (it == messages.end()) {
        messages.append(msg);
        m_dirty = true;
        return StoreResult::Updated;
    }
    else if (isFullVersionOf(*it, msg)) {
        *it = msg;
        m_dirty = true;
        return StoreResult::Updated;
    }
    else {
        return StoreResult::AlreadyKnown;
    }
}



