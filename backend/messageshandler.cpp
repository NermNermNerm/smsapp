#include "messageshandler.h"
#include "dbus.h"
#include <QSharedPointer>

static const QString orgKdeConnect = "org.kde.kdeconnect"; // "org.fake.kdeconnect";

MessagesHandler::MessagesHandler(const QString &deviceID, QObject *parent)
    : QObject{parent}, m_deviceID(deviceID), m_cacheManager(deviceID, this), m_attachmentCache(deviceID)
{
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::conversationLoaded,
                     this, &MessagesHandler::onConversationLoaded);
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::conversationCreated,
                     this, &MessagesHandler::onConversationCreated);
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::conversationUpdated,
                     this, &MessagesHandler::onConversationUpdated);
    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::attachmentReceived,
                     this, &MessagesHandler::onAttachmentReceived);

    m_retryTimer.setInterval(1000);   // 1 second
    m_retryTimer.setSingleShot(true);

    connect(&m_outgoingTimeoutTimer, &QTimer::timeout, this, &MessagesHandler::checkOutgoingTimeouts);
    m_outgoingTimeoutTimer.setInterval(1000);
    m_outgoingTimeoutTimer.setSingleShot(false);
    m_outgoingTimeoutTimer.start();

    connect(&m_retryTimer, &QTimer::timeout, this, &MessagesHandler::attemptRequestAllThreads);
    attemptRequestAllThreads();

    // We're setting this now to avoid giving a down reading when we can be pretty sure things
    // are in a good state since we got a positive signal on device reachability to get here.
    noteDaemonActivity();

    QObject::connect(&dbus::conversations(m_deviceID), &org::kde::kdeconnect::conversations::attachmentReceived,
                     this, [&](QString filePath, QString fileName) {
        qWarning() << "Got attachment, filePath:" << filePath << "fileName:" << fileName;
    });
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
        resolvePendingOutgoing(message);
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
        resolvePendingOutgoing(message);
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

void MessagesHandler::sendMessage(qint64 conversationID, const QString &body, const QVector<QUrl> &attachments)
{
    if (!dbus::device(m_deviceID).isReachable()) {
        emit messageDeliveryFailed(conversationID);
        return;
    }

    QVariantList dbusAttachments;
    for (const QUrl &url: attachments)
        dbusAttachments.append(url.toLocalFile());

    Q_ASSERT(!m_conversationsWithOutgoingMessages.contains(conversationID));
    QDBusPendingReply<void> reply = dbus::conversations(m_deviceID).replyToConversation(conversationID, body, dbusAttachments);

    // Watch for transport-level failure
    auto watcher = QSharedPointer<QDBusPendingCallWatcher>(new QDBusPendingCallWatcher(reply, this));
    connect(watcher.data(), &QDBusPendingCallWatcher::finished, this,
            [conversationID, watcher, this]() {
                Q_ASSERT(!this->m_deviceID.isEmpty()); // shenanigans to avoid a clang-null-deref false positive.
                if (watcher.data()->isError()) {
                    emit messageDeliveryFailed(conversationID);
                } else {
                    QMutexLocker locker(&m_outgoingMutex);
                    m_conversationsWithOutgoingMessages[conversationID] = QDateTime::currentDateTimeUtc();
                }
            });
}

void MessagesHandler::resolvePendingOutgoing(const ConversationMessage &message)
{
    if (!message.isOutgoing())
        return;

    bool shouldEmit = false;
    {
        QMutexLocker locker(&m_outgoingMutex);
        shouldEmit = m_conversationsWithOutgoingMessages.remove(message.threadID());
    }

    if (shouldEmit) {
        emit messageDelivered(message.threadID());
    }
}

void MessagesHandler::checkOutgoingTimeouts()
{
    const auto now = QDateTime::currentDateTimeUtc();
    QList<qint64> expired;

    {
        QMutexLocker locker(&m_outgoingMutex);

        for (auto it = m_conversationsWithOutgoingMessages.begin();
             it != m_conversationsWithOutgoingMessages.end(); )
        {
            if (it.value().msecsTo(now) > WaitTimeInMsForMessageDelivery) {
                expired.append(it.key());
                it = m_conversationsWithOutgoingMessages.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (qint64 conversationID : expired)
        emit messageDeliveryFailed(conversationID);
}

QString MessagesHandler::tryGetCachedAttachment(const Attachment &attachment) const
{
    assertOnMyThread();
    qInfo() << Q_FUNC_INFO << attachment.uniqueIdentifier();
    return m_attachmentCache.tryGetCachedAttachment(attachment);
}

void MessagesHandler::requestAttachment(const Attachment &attachment, const QString &path)
{
    assertOnMyThread();

    qInfo() << Q_FUNC_INFO << attachment.uniqueIdentifier() << (path.isEmpty() ? "''" : path);
    const bool queueWasEmpty = m_attachmentRequests.isEmpty();
    m_attachmentRequests.enqueue(AttachmentRequestQueueItem{attachment, path});

    processCachedItemsInQueue();
    if (queueWasEmpty) {
        sendHeadRequest();
    }
}

void MessagesHandler::onAttachmentReceived(const QString &path, const QString &uniqueId)
{
    assertOnMyThread();

    qInfo() << Q_FUNC_INFO << path << uniqueId;
    noteDaemonActivity();

    if (m_attachmentRequests.isEmpty()) {
        qWarning() << "Unexpected attachmentReceived:" << uniqueId;
        return;
    }

    AttachmentRequestQueueItem req = m_attachmentRequests.head();
    if (req.attachment.uniqueIdentifier() != uniqueId) {
        // For this and the previous message, we *could* search the queue to see if it's out of order, which would
        // only happen if there was another agent on the dbus.  It doesn't seem worth defending against that at this point.
        qWarning() << "attachmentReceived mismatch:" << uniqueId
                   << "expected:" << req.attachment.uniqueIdentifier();
        return;
    }

    // Determine final target path
    QString targetPath;

    if (req.pathOverride.isEmpty()) {
        targetPath = m_attachmentCache.emplaceFile(req.attachment, path);
    } else {
        targetPath = req.pathOverride;
        if (QFile::exists(targetPath)) QFile::remove(targetPath);
        if (QFile::copy(path, targetPath)) {
            m_completedDownloads[req.attachment.uniqueIdentifier()] = targetPath;
        }
        else {
            targetPath = "";
            qWarning() << "Failed to copy" << path << "to" << targetPath;
        }
    }
    m_attachmentRequests.dequeue();

    emit attachmentRecieved(req.attachment, targetPath, req.pathOverride.isEmpty());

    processCachedItemsInQueue();
    if (!m_attachmentRequests.isEmpty())
        sendHeadRequest();
}

void MessagesHandler::processCachedItemsInQueue()
{
    assertOnMyThread();

    for (int i = 0; i < m_attachmentRequests.size(); ) {

        const AttachmentRequestQueueItem req = m_attachmentRequests.at(i);
        const QString cachedPath = m_attachmentCache.tryGetCachedAttachment(req.attachment);

        if (cachedPath.isEmpty()) {
            ++i;
        }
        else {
            QString finalPath = cachedPath;
            bool isInCache = !req.pathOverride.isEmpty();

            if (!isInCache) {
                finalPath = req.pathOverride;

                if (QFile::exists(finalPath))
                    QFile::remove(finalPath);

                if (!QFile::copy(cachedPath, finalPath)) {
                    qWarning() << "Failed to copy cached attachment from"
                               << cachedPath << "to" << finalPath;
                    finalPath.clear();
                }

                isInCache = false;
            }

            m_attachmentRequests.removeAt(i);
            emit attachmentRecieved(req.attachment, finalPath, isInCache);

            // Do NOT increment i — we removed the element at i,
            // so the next element has now shifted into index i.
        }
    }
}

void MessagesHandler::sendHeadRequest()
{
    assertOnMyThread();

    if (m_attachmentRequests.isEmpty())
        return;

    // Now the head is not in cache → send the DBus request.
    const AttachmentRequestQueueItem &req = m_attachmentRequests.head();

    auto reply = dbus::conversations(m_deviceID)
                     .requestAttachmentFile(req.attachment.partID(),
                                            req.attachment.uniqueIdentifier());

    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    // Wrap it in a QSharedPointer ONLY to silence the analyzer. The custom deleter does nothing because Qt owns the lifetime.
    QSharedPointer<QDBusPendingCallWatcher> analyzerShutup(watcher, [](QDBusPendingCallWatcher*){});

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher]() {
                watcher->deleteLater();
                if (watcher->isError()) {
                    const auto failed = m_attachmentRequests.dequeue();
                    emit attachmentRecieved(failed.attachment, QString(), false);
                    sendHeadRequest();
                }
            });
}

bool MessagesHandler::isDownloadUnderway(const Attachment &attachment) const
{
    assertOnMyThread();

    return std::any_of(m_attachmentRequests.begin(),
                       m_attachmentRequests.end(),
                       [&](const AttachmentRequestQueueItem &item) {
                           return !item.pathOverride.isEmpty()
                                  && item.attachment.uniqueIdentifier() == attachment.uniqueIdentifier();
                       });
}

QString MessagesHandler::tryFindCompletedDownload(const Attachment &attachment) const
{
    assertOnMyThread();

    return m_completedDownloads.value(attachment.uniqueIdentifier());
}

void MessagesHandler::assertOnMyThread() const
{
    Q_ASSERT(QThread::currentThread() == this->thread());
}
