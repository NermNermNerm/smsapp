#include "messageshandler.h"

static const QString orgKdeConnect = "org.kde.kdeconnect"; // "org.fake.kdeconnect";

MessagesHandler::MessagesHandler(const QString &deviceID, QObject *parent)
    : QObject{parent},
    m_device(orgKdeConnect, "/modules/kdeconnect/devices/" + deviceID, QDBusConnection::sessionBus()),
    m_conversations(orgKdeConnect, "/modules/kdeconnect/devices/" + deviceID, QDBusConnection::sessionBus()),
    m_deviceID(deviceID)
{
}

void MessagesHandler::startListening()
{
    Q_ASSERT(!m_startListeningCalled);
    if (m_startListeningCalled)
    {
        return;
    }
    m_startListeningCalled = true;

    m_cacheManager.load(m_deviceID);

    QObject::connect(&m_conversations, &org::kde::kdeconnect::conversations::conversationLoaded,
                     this, &MessagesHandler::onConversationLoaded);
    QObject::connect(&m_conversations, &org::kde::kdeconnect::conversations::conversationCreated,
                     this, &MessagesHandler::onConversationCreated);
    QObject::connect(&m_conversations, &org::kde::kdeconnect::conversations::conversationUpdated,
                     this, &MessagesHandler::onConversationUpdated);

    m_conversations.requestAllConversationThreads();

    // We're setting this now to avoid giving a down reading when we can be pretty sure things
    // are in a good state since we got a positive signal on device reachability to get here.
    noteDaemonActivity();
}

void MessagesHandler::requestConversationItems(
    qint64 conversationID,
    int startingIndex,
    int endingIndex,
    MessageAvailableCallback onMessageAvailable,
    FailureCallback onFailure)
{
    // Process any we already have in cache
    int firstUncachedIndex = -1;
    qint64 expectedDateMinimum = 0;
    for (int i = startingIndex; i <= endingIndex; ++i) {
        ConversationMessage cachedMessage;
        if (m_cacheManager.tryGetMessage(conversationID, i, cachedMessage)) {
            onMessageAvailable(i, cachedMessage);
            expectedDateMinimum = cachedMessage.date();
        }
        else {
            firstUncachedIndex = i;
            break;
        }
    }

    if (firstUncachedIndex >= 0) {
        // We didn't find all the requested values in the cache.

        // Check and see if there are some  at the far side of the range already.
        qint64 expectedDateMaximum = std::numeric_limits<qint64>::max();
        int lastUncachedIndex = endingIndex; // Note that we're guaranteed to hit the 'else' condition below
        for (int i = endingIndex; i >= startingIndex; --i) {
            ConversationMessage cachedMessage;
            if (m_cacheManager.tryGetMessage(conversationID, i, cachedMessage)) {
                expectedDateMaximum = cachedMessage.date();
                onMessageAvailable(i, cachedMessage);
            }
            else {
                lastUncachedIndex = i;
                break;
            }
        }

        m_pendingRequests.enqueue(PendingRequest{
            .conversationID      = conversationID,
            .startingIndex       = firstUncachedIndex,
            .endingIndex         = lastUncachedIndex,
            .onMessageAvailable  = std::move(onMessageAvailable),
            .onFailure           = std::move(onFailure),
            .expectedDateMinimum = expectedDateMinimum,
            .expectedDateMaximum = expectedDateMaximum
        });

        if (m_pendingRequests.length() == 1) {
            const PendingRequest& headRequest = m_pendingRequests.head();
            m_conversations.requestConversation(headRequest.conversationID, headRequest.startingIndex, headRequest.endingIndex);
        }
    }
}

int MessagesHandler::getNumberOfMessagesInConversation(qint64 conversationID)
{
    int messageCount = 0;
    bool isKnownConversation = m_cacheManager.tryGetMessageCount(conversationID, messageCount);
    Q_ASSERT(isKnownConversation);
    return messageCount;
}


void MessagesHandler::onConversationLoaded(qint64 threadID, qint64 messageCount)
{
    noteDaemonActivity();

    qDebug() << Q_FUNC_INFO << threadID;
    m_cacheManager.updateMessageCount(threadID, messageCount);

    if (threadID == m_messageBeingCreated.threadID())
    {
        m_cacheManager.storeMessage(threadID, 1, m_messageBeingCreated);
        m_messageBeingCreated = ConversationMessage();
    }

    // Any ConversationCreated message is expected to be immediately followed by an onConversationLoaded signal.
    Q_ASSERT(m_messageBeingCreated.threadID() == 0);

    emit conversationMessageCountChanged(threadID, messageCount);
}

void MessagesHandler::onConversationUpdated(const QDBusVariant &msg)
{
    noteDaemonActivity();
    ConversationMessage message = ConversationMessage::fromDBus(msg);
    qDebug() << Q_FUNC_INFO << message.threadID() << message.body();

    // Any ConversationCreated message is expected to be immediately followed by an onConversationLoaded signal.
    Q_ASSERT(m_messageBeingCreated.threadID() == 0);

    // Is this exactly the message we're requesting... probably.
    PendingRequest* headRequest = m_pendingRequests.empty() ? nullptr : &m_pendingRequests.head();
    if (headRequest
        && headRequest->conversationID == message.threadID()
        && headRequest->expectedDateMinimum <= message.date()
        && message.date() <= headRequest->expectedDateMaximum)
    {
        int index = headRequest->startingIndex;
        m_cacheManager.storeMessage(message.threadID(), index, message);
        headRequest->onMessageAvailable(index, message);
        if (headRequest->startingIndex < headRequest->endingIndex) {
            headRequest->startingIndex += 1;
        }
        else {
            m_pendingRequests.pop_front();
            if (!m_pendingRequests.empty()) {
                PendingRequest* newHeadRequest = &m_pendingRequests.head();
                m_conversations.requestConversation(newHeadRequest->conversationID, newHeadRequest->startingIndex, newHeadRequest->endingIndex);
            }
        }
    }
    else if (m_cacheManager.isNewerThanCached(message))
    {
        // ... it's a new text message has arrived on an existing conversation.
        int messageCount;
        bool isInCache = m_cacheManager.tryGetMessageCount(message.threadID(), messageCount);
        // The daemon shouldn't send us messages it hasn't sent us conversationLoaded messages for.
        Q_ASSERT(isInCache);
        // ... but it conceivably could if a message arrived while it's spewing conversationUpdated while
        // processing the requestAllConversationThreads request.  We'll handle it in release mode, but
        // it's so rare, it seems worth an assert.
        if (isInCache) {
            m_cacheManager.updateMessageCount(message.threadID(), messageCount+1);
            m_cacheManager.storeMessage(message.threadID(), 1, message); // Note: indices are 1-based.
            emit conversationMessageCountChanged(message.threadID(), messageCount+1);
        }
        // else ignore it, as we'll have to pick it up when we finally get the loaded message.
        else {
            qDebug() << "  !Ignored message from a thread that we haven't got in our cache";
        }
    }
    else {
        qDebug() << "  !Ignored message that appears to be aimed at another SMS app on the kdbus";
    }

    // TODO: Consider going into some kind of error state if we haven't made progress on the
    //  head request after a while.
}

void MessagesHandler::onConversationCreated(const QDBusVariant &msg)
{
    noteDaemonActivity();
    ConversationMessage message = ConversationMessage::fromDBus(msg);
    qDebug() << Q_FUNC_INFO << message.threadID() << message.body();

    // If this asserts, the daemon didn't send us a conversationLoaded message after the last created one.
    // ?? Perhaps this is possible if two brand new conversations get created at the same instant and the
    // daemon handles them on separate threads?
    Q_ASSERT(m_messageBeingCreated.threadID() == 0);

    m_messageBeingCreated = message;
}