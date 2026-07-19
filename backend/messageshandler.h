#include "pch.h"
#pragma once
#include "cachemanager.h"
#include "attachmentcache.h"

class ConversationMessage;
class QDBusPendingCallWatcher;

//  Manages all conversations with the kde connect daemon regarding SMS messages on a specific device.
class MessagesHandler : public QObject
{
    Q_OBJECT

    const int WaitTimeInMsForMessageDelivery = 10000;

public:
    explicit MessagesHandler(const QString &deviceId, QObject *parent = nullptr);

    // Call this after construction to attach to the dbus, load the cache, and start forwarding events.
    // This should only be called once.
    void startListening();

    void requestConversationItems(qint64 conversationID);
    QDateTime lastDaemonActivityUtc() const { return m_lastActivity; }
    const QString &deviceID() const { return m_deviceID; }
    bool isConversationKnown(qint64 conversationID) const { return m_knownThreads.contains(conversationID); }

    QVector<ConversationMessage> getAllConversationMessages() const
        { return m_cacheManager.getAllConversationMessages(); }
    QVector<ConversationMessage> getConversationMessages(qint64 conversationID) const
        { return m_cacheManager.getConversationMessages(conversationID); }

    void sendMessage(qint64 conversationID, const QString &messageBody, const QVector<QUrl> &attachments);
    bool hasUndeliveredOutgoing(qint64 conversationID) { return m_conversationsWithOutgoingMessages.contains(conversationID); }

    /** @brief If the given attachment is in the cache, this will return the full
      * path to the file.  If it is not in the cache already, it will return "". */
    QString tryGetCachedAttachment(const Attachment &attachment) const;

    /** @brief This will initiate a request to the server to download the attachment.
     *    When it is loaded, the attachmentRetrieved signal will be emitted.
     *  @param path If this is specified, it will not be downloaded to the cache but instead
     *    directly to this location, bypassing the cache entirely.
     *  @remarks If the file is actually in cache, the attachmentRetrieved event
     *    will be fired immediately. */
    void requestAttachment(const Attachment &attachment, const QString &path = "");

    /** @brief Returns true if requestAttachment has been called on the given attachment
     *    with a 'path' argument and it hasn't yet found its way to its target location. */
    bool isDownloadUnderway(const Attachment &attachment) const;

    /** @brief If requestAttachment has been called on the given attachment
     *    and it has finished downloading this returns the path it went to.
     *    Else it returns "". */
    QString tryFindCompletedDownload(const Attachment &attachment) const;

signals:
    void conversationMessageChanged(const ConversationMessage &updatedMessage);
    void conversationBecomesKnown(qint64 conversationID);
    void conversationDeleted(qint64 conversationId);

    void messageDelivered(qint64 conversationID);
    void messageDeliveryFailed(qint64 conversationID);

    /**
     * @brief Signals that a requested attachment has been fully downloaded.
     * @param path The path where the file exists
     * @param isInCache This will be true unless the requestAttachment call that spawned
     *   it specified that it should be put somewhere else.
     */
    void attachmentRecieved(const Attachment &attachment, const QString &path, bool isInCache);

    // Note: conversationCreated is handled internally, as it's really just the first message
    // in a two message conversation.  Our signal will be conversationLoaded, when conversationCreated
    // will have already been fully processed.

private:
    void onConversationLoaded(qint64 conversationID, qint64 messageCount);
    void onConversationUpdated(const QDBusVariant &msg);
    void onConversationCreated(const QDBusVariant &msg);
    void onAttachmentReceived(const QString &path, const QString &uniqueID);

    void attemptRequestAllThreads();
    void onRequestAllThreadsFinished(QDBusPendingCallWatcher *watcher);

    void noteDaemonActivity() { m_lastActivity = QDateTime::currentDateTimeUtc(); }
    void markConversationKnown(qint64 conversationID);

    void resolvePendingOutgoing(const ConversationMessage &message);
    void checkOutgoingTimeouts();

    /** @brief For code that would have a problem if run concurrently, but we know that it will only
     *  ever run on this object's thread, use this to assert that it's not called outside of that assumption. */
    void assertOnMyThread() const;

    struct AttachmentRequestQueueItem {
        Attachment attachment;
        QString pathOverride;
    };
    QQueue<AttachmentRequestQueueItem> m_attachmentRequests;

    void processCachedItemsInQueue();
    void sendHeadRequest();

    QDateTime m_lastActivity;
    const QString m_deviceID;
    QTimer m_retryTimer;
    qint64 m_stableThreadTime = 0;

    QQueue<qint64> m_conversationsNeedingUpdate;
    QSet<qint64> m_knownThreads;

    CacheManager m_cacheManager;
    QHash<qint64,QDateTime> m_conversationsWithOutgoingMessages;
    mutable QMutex m_outgoingMutex;
    QTimer m_outgoingTimeoutTimer;
    AttachmentCache m_attachmentCache;

    QMap<QString,QString> m_completedDownloads;
};
