#pragma once
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QDateTime>
#include <QTimer>
#include "cachemanager.h"

class ConversationMessage;
class QDBusPendingCallWatcher;

//  Manages all conversations with the kde connect daemon regarding SMS messages on a specific device.
class MessagesHandler : public QObject
{
    Q_OBJECT
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

signals:
    void conversationMessageChanged(const ConversationMessage &updatedMessage);
    void conversationBecomesKnown(qint64 conversationID);
    void conversationDeleted(qint64 conversationId);

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
    void markConversationKnown(qint64 conversationID);

    QDateTime m_lastActivity;
    QString m_deviceID;
    QTimer m_retryTimer;
    qint64 m_stableThreadTime = 0;

    QQueue<qint64> m_conversationsNeedingUpdate;
    QSet<qint64> m_knownThreads;

    CacheManager m_cacheManager;
};
