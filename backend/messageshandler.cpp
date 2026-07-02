#include "messageshandler.h"
#include "dbus.h"

static const QString orgKdeConnect = "org.kde.kdeconnect"; // "org.fake.kdeconnect";

MessagesHandler::MessagesHandler(const QString &deviceID, QObject *parent)
    : QObject{parent}, m_deviceID(deviceID), m_cacheManager(deviceID, this)
{
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::conversationLoaded,
                     this, &MessagesHandler::onConversationLoaded);
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::conversationCreated,
                     this, &MessagesHandler::onConversationCreated);
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::conversationUpdated,
                     this, &MessagesHandler::onConversationUpdated);

    m_retryTimer.setInterval(1000);   // 1 second
    m_retryTimer.setSingleShot(true);

    connect(&m_retryTimer, &QTimer::timeout, this, &MessagesHandler::attemptRequestAllThreads);
    attemptRequestAllThreads();

    // We're setting this now to avoid giving a down reading when we can be pretty sure things
    // are in a good state since we got a positive signal on device reachability to get here.
    noteDaemonActivity();
}

void MessagesHandler::attemptRequestAllThreads()
{
    QDBusPendingReply<> reply = dbus::conversations(m_deviceID).requestAllConversationThreads();

    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &MessagesHandler::onRequestAllThreadsFinished);
}

void MessagesHandler::onRequestAllThreadsFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError()) {
        qWarning() << "requestAllConversationThreads failed:" << reply.error().message();

        // Try again in 1 second
        m_retryTimer.start();
    }
    else {
        qDebug() << "requestAllConversationThreads succeeded";
    }
}

void MessagesHandler::requestConversationItems(qint64 conversationID)
{
    dbus::conversations(m_deviceID).requestConversation(conversationID, 0, 9999);
}

void MessagesHandler::onConversationLoaded(qint64 threadID, qint64 messageCount)
{
    noteDaemonActivity();
    qDebug() << Q_FUNC_INFO << threadID << messageCount;
    if (messageCount == 0) {
        m_cacheManager.deleteConversation(threadID);
        m_knownThreads.remove(threadID);
        emit conversationDeleted(threadID);
    }
    else if (!m_knownThreads.contains(threadID)) {
        if (m_stableThreadTime > 0 && m_cacheManager.isCachedThreadOlderThan(threadID, m_stableThreadTime)) {
            markConversationKnown(threadID);
        }
        else if (!m_conversationsNeedingUpdate.contains(threadID)) { // <- should always be true unless there's other client activity.
            // Even though we have evidence that the thread is known, we don't mark it known here
            // and instead wait for its 'updated' message to come through in order to avoid the risk
            // of the thread-viewing window sending a requestConversationItems call simultaneous with
            // the one we're sending here.
            bool queueWasEmpty = m_conversationsNeedingUpdate.isEmpty();
            m_conversationsNeedingUpdate.enqueue(threadID);
            if (queueWasEmpty) {
                dbus::conversations(m_deviceID).requestConversation(threadID, 0, 0);
            }
        }
    }
}

void MessagesHandler::onConversationUpdated(const QDBusVariant &msg)
{
    noteDaemonActivity();
    ConversationMessage message = ConversationMessage::fromDBus(msg);
    qDebug() << Q_FUNC_INFO << message.threadID() << message.body();
    auto storeCacheResult = m_cacheManager.storeMessage(message);
    if (storeCacheResult == CacheManager::StoreResult::Updated) {
        emit conversationMessageChanged(message);
    }
    if (!m_conversationsNeedingUpdate.empty() && message.threadID() == m_conversationsNeedingUpdate.head()) {
        if (storeCacheResult == CacheManager::StoreResult::AlreadyKnown) {
            m_stableThreadTime = message.date();
        }

        m_conversationsNeedingUpdate.pop_front();
        //while the head of conversationNeedingUpdate is the id of a thread in the cache whose youngest timestamp is older than stableThreadTime
        while (!m_conversationsNeedingUpdate.empty()
               && m_cacheManager.isCachedThreadOlderThan(m_conversationsNeedingUpdate.head(), m_stableThreadTime)) {
            markConversationKnown(m_conversationsNeedingUpdate.head());
            m_conversationsNeedingUpdate.pop_front();
        }

        // If there is still work to do, request the next head
        if (!m_conversationsNeedingUpdate.empty()) {
            dbus::conversations(m_deviceID).requestConversation(m_conversationsNeedingUpdate.head(), 0, 0);
        }
    }

    markConversationKnown(message.threadID());
}

void MessagesHandler::onConversationCreated(const QDBusVariant &msg)
{
    noteDaemonActivity();
    ConversationMessage message = ConversationMessage::fromDBus(msg);
    qDebug() << Q_FUNC_INFO << message.threadID() << message.body();
    if (m_cacheManager.storeMessage(message) == CacheManager::StoreResult::Updated) {
        emit conversationMessageChanged(message);
    }
    markConversationKnown(message.threadID());
}

void MessagesHandler::markConversationKnown(qint64 conversationID)
{
    if (!m_knownThreads.contains(conversationID)) {
        m_knownThreads.insert(conversationID);
        emit conversationBecomesKnown(conversationID);
    }
}
