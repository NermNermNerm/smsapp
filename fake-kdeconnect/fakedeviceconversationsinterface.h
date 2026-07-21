#ifndef FAKESMS_H
#define FAKESMS_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include "kdeconnect_interfaces/conversationmessage.h"
#include <qdbusabstractadaptor.h>
#include <qdbuspendingreply.h>

struct DeviceConfig;

/*
 * Fake implementation class for interface org.kde.kdeconnect.device.conversations
 */
class FakeDeviceConversationsInterface: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.conversations")

public:
    static inline const char *staticInterfaceName()
    { return "org.kde.kdeconnect.device.conversations"; }

    // This simulation doesn't accurately track what the real daemon does with the 'messageCount'
    //  parameter of conversationLoaded, and the sms client that this is built to serve doesn't
    //  utilize it (because the real daemon spews something of dubious, if any, value.)
    const qint64 nonsenseCountValue = 9876;

    int interval() const { return m_intervalMs; }
    void setInterval(int mSec);
    int sendInterval() const { return m_sendIntervalMs; }
    void setSendInterval(int mSec);
    int attachmentInterval() const { return m_attachmentIntervalMs; }
    void setAttachmentInterval(int mSec) { m_attachmentIntervalMs = mSec; }

public:
    FakeDeviceConversationsInterface(DeviceConfig *deviceConfig, QObject *parent = nullptr);

    void simulateIncomingMessage(const ConversationMessage &m);

    void reset();

public Q_SLOTS: // METHODS
    void replyToConversation(qlonglong conversationID, const QString &message, const QVariantList &attachmentUrls);
    void requestAllConversationThreads();
    void requestConversation(qlonglong conversationID, int start, int end);
    void sendWithoutConversation(const QVariantList &addressList, const QString &message, const QVariantList &attachmentUrls);
    void requestAttachmentFile(qlonglong partID, const QString &uniqueIdentifier);

Q_SIGNALS: // SIGNALS
    void conversationCreated(const QDBusVariant &msg);
    void conversationLoaded(qlonglong conversationID, qulonglong messageCount);
    void conversationRemoved(qlonglong conversationID);
    void conversationUpdated(const QDBusVariant &msg);
    void attachmentReceived(const QString &filePath, const QString &fileName);

private:
    ConversationMessage metadataTruncate(const ConversationMessage &msg);
    void emitNextThread();

    void processIncomingQueue();
    void deliverIncomingMessageNow(const ConversationMessage &m);

    DeviceConfig *m_deviceConfig;
    QTimer *m_enumerationTimer;

    /** This is used by requestAllConversationThreads and emitNextThread to spew the threads
      * in a realistic way. */
    std::vector<ConversationMessage> m_enumerationHeads;
    size_t m_nextIndex = 0;

    // This represents all the threads that the simulated daemon would have loaded into its cache,
    // and therefor would send new activity on in the 'conversationUpdated' signal.
    QSet<qint64> m_knownThreads;
    QSet<qint64> m_fullyLoadedThreads;

    QQueue<ConversationMessage> m_incomingQueue;
    QMutex m_incomingMutex;
    QMutex m_enumerationMutex;
    QTimer *m_incomingTimer;
    int m_intervalMs = 50;
    int m_sendIntervalMs = 50;
    int m_attachmentIntervalMs = 4000;
};

#endif // FAKESMS_H
