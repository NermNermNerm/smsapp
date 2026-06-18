#pragma once

#include <QObject>
#include <QTimer>
#include <QDBusInterface>
#include <QDBusReply>
#include "kdeconnect_proxy.h"
#include "conversationlistmodel.h"

class SmsBackend : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString deviceStatus READ deviceStatus NOTIFY deviceStatusChanged)
    Q_PROPERTY(QString extendedStatus READ extendedStatus NOTIFY extendedStatusChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(ConversationListModel *conversationList READ conversationList CONSTANT)

public:
    explicit SmsBackend(QObject *parent = nullptr);

    enum class Status {
        Ok,
        DaemonUnavailable,
        DeviceUnreachable,
        SmsPluginUnavailable,
        SmsInterfaceInvalid,
        DeviceRemoved,
        NoPrimaryDevice
    };
    Q_ENUM(Status)

    QString deviceStatus() const { return m_deviceStatus; }
    Status rawDeviceStatus() const { return m_rawDeviceStatus; }
    QString extendedStatus() const { return m_extendedStatus; }
    QString deviceName() const { return m_deviceName; }
    ConversationListModel *conversationList() { return &m_conversationList; }
    int unreadMessageCount() const { return 0; } // TODO:

signals:
    void deviceStatusChanged();
    void extendedStatusChanged();
    void deviceNameChanged();
    void unreadMessageCountChanged();

private slots:
    void poll();

private:
    void setStatus(Status s);
    void attachToSmsInterface();
    bool validateExistingDevice();
    void discoverNewDevice();

    void handleConversationLoaded(qint64 threadId, qint64 messageCount);
    void handleConversationUpdate(const QDBusVariant &msg);
    void handleConversationCreated(const QDBusVariant &msg);

    QString m_deviceId;       // persisted primary device
    QString m_deviceName;
    Status m_rawDeviceStatus;
    QString m_deviceStatus;
    QString m_extendedStatus;

    ConversationListModel m_conversationList{this};

    org::kde::kdeconnect::daemon m_daemon;

    // These are initialized once the device is identified.
    org::kde::kdeconnect::device *m_device = nullptr;
    org::kde::kdeconnect::conversations *m_conversations = nullptr;
    // org::kde::kdeconnect::sms *m_sms;

    qint64 m_lastMessageFromDaemonRecievedTime = 0;

    static constexpr int PollIntervalInMs = 2000;
    static constexpr int SilentTimeInMsBeforePoll = 2000;
};
