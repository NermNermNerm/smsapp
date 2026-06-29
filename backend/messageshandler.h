#pragma once
#include <QObject>
#include <QQueue>
#include <QDateTime>
#include "kdeconnect_proxy.h"
#include "cachemanager.h"

class ConversationMessage;

//  Manages all conversations with the kde connect daemon regarding SMS messages on a specific device.
class MessagesHandler : public QObject
{
    Q_OBJECT
public:
    using MessageAvailableCallback = std::function<void(int requestedIndex,  const ConversationMessage &item)>;
    using FailureCallback = std::function<void(qint64 conversationID, int requestedIndex)>;

    explicit MessagesHandler(const QString &deviceId, QObject *parent = nullptr);

    // Call this after construction to attach to the dbus, load the cache, and start forwarding events.
    // This should only be called once.
    void startListening();

    void requestConversationItems(qint64 conversationID, int startingIndex, int endingIndex, MessageAvailableCallback onMessageAvailable, FailureCallback onFailure = nullptr);
    void requestConversationItem(qint64 conversationID, int singleIndex, MessageAvailableCallback onCompletion, FailureCallback onFailure = nullptr) {
        requestConversationItems(conversationID, singleIndex, singleIndex, onCompletion, onFailure);
    }

    int getNumberOfMessagesInConversation(qint64 conversationID);
    QDateTime lastDaemonActivityUtc() const { return m_lastActivity; }

    const QString &deviceID() const { return m_deviceID; }

    // TODO: This probably isn't the right signature, as we should probably return an opaque "RequestID" type so that
    //   callers can cancel a particular request.
    // void cancelPendingRequestsForConversation(qint64 conversationID);

    int unreadMessageCount() const { return 0; } // TODO

signals:
    // A new text has arrived for a conversation
    void conversationMessageCountChanged(qint64 conversationID, int messageCount);

    // Note: conversationCreated is handled internally, as it's really just the first message
    // in a two message conversation.  Our signal will be conversationLoaded, when conversationCreated
    // will have already been fully processed.

private:
    void onConversationLoaded(qint64 conversationID, qint64 messageCount);
    void onConversationUpdated(const QDBusVariant &msg);
    void onConversationCreated(const QDBusVariant &msg);

    void attemptRequestAllThreads();
    void onRequestAllThreadsFinished(QDBusPendingCallWatcher *watcher);

    void noteDaemonActivity() { m_lastActivity = QDateTime::currentDateTimeUtc(); }

    struct PendingRequest {
        qint64 conversationID;
        int startingIndex;
        int endingIndex;
        MessageAvailableCallback onMessageAvailable;
        FailureCallback onFailure;
        qint64 expectedDateMinimum;
        qint64 expectedDateMaximum;
    };

    QDateTime m_lastActivity;
    QString m_deviceID;
    QTimer m_retryTimer;

    // If m_pendingRequests is empty, that means no requests have been sent out on dbus.
    //  Otherwise, the entry at the tip of the queue is the one we're waiting for dbus to respond to.
    QQueue<PendingRequest> m_pendingRequests;

    CacheManager m_cacheManager;

    // m_deviceManager.conversationCreated() happens first - it gives us this message, but we need to know
    // how many messages are in the new thread before we can add it to the cache.  For that, we need to await
    // the conversationLoaded signal.
    ConversationMessage m_messageBeingCreated;

    bool m_startListeningCalled = false;
};
