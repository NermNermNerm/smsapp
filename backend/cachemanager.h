#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QReadWriteLock>

#include "kdeconnect_interfaces/conversationmessage.h"

class QTimer;

class CacheManager : public QObject
{
    Q_OBJECT

public:
    explicit CacheManager(QObject *parent = nullptr);
    ~CacheManager() override;

    // Load/save persistent JSON cache
    void load(const QString &deviceId);
    void saveNow();

    // Update the daemon-reported messageCount for a thread.
    // Shifts cached indices upward if daemonCount increased.
    void updateMessageCount(qint64 threadId, int daemonCount);

    // Try-get pattern for retrieving a cached message by index.
    bool tryGetMessage(qint64 threadId, int index, ConversationMessage &out) const;

    // Store a message at a given index.
    // Backend assigns the index; CacheManager does not validate it.
    void storeMessage(qint64 threadId, int index, const ConversationMessage &msg);

    void clearThread(qint64 threadId);

    bool hasMessage(qint64 threadId, const ConversationMessage &msg) const;

    bool isNewerThanCached(const ConversationMessage &msg) const;

    bool tryGetMessageCount(qint64 threadId, int &messageCount) const;

private slots:
    void saveIfDirty();

private:
    bool saveUnlocked() const;

    struct ThreadData {
        int messageCount = 0; // daemon's count
        QMap<int, ConversationMessage> messages; // index → message
    };

    QString m_cachePath;
    mutable QReadWriteLock m_lock;
    QMap<qint64, ThreadData> m_threads;
    bool m_dirty = false;
    QTimer *m_timer = nullptr;
};
