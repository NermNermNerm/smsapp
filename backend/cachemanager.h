#pragma once


#include "kdeconnect_interfaces/conversationmessage.h"

class CacheManager : public QObject
{
    Q_OBJECT

public:
    explicit CacheManager(const QString &deviceId, QObject *parent = nullptr);

    void save();

    enum class StoreResult {
        // The cache contained identical information to what's given here
        AlreadyKnown,
        // The message was either changed or inserted
        Updated,
    };
    StoreResult storeMessage(const ConversationMessage &msg);

    /**
     * @brief Returns true if the cached thread for the given conversation id exists and all
     *   the messages are older than the given time.  cutoffTime is in the same units as
     *   ConversationMessage::date().
     */
    bool isCachedThreadOlderThan(qint64 conversationID, qint64 cutoffTime) const;
    void deleteConversation(qint64 conversationID);
    QVector<ConversationMessage> getAllConversationMessages() const;
    QVector<ConversationMessage> getConversationMessages(qint64 conversationID) const;

private:
    bool doSave() const;
    void doLoad(const QString &deviceId);

    QString m_cachePath;
    mutable QReadWriteLock m_lock;
    QMap<qint64, QVector<ConversationMessage>> m_threads;
    bool m_dirty = false;
    QTimer m_timer;
};
