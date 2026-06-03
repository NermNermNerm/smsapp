#pragma once

#include <QObject>
#include <QTimer>
#include <QDBusInterface>
#include <QDBusReply>

class SmsBackend : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString deviceStatus READ deviceStatus NOTIFY deviceStatusChanged)
    Q_PROPERTY(QString extendedStatus READ extendedStatus NOTIFY extendedStatusChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(QString lastSender READ lastSender NOTIFY lastMessageChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

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
    QString extendedStatus() const { return m_extendedStatus; }
    QString deviceName() const { return m_deviceName; }
    QString lastSender() const { return m_lastSender; }
    QString lastMessage() const { return m_lastMessage; }

signals:
    void deviceStatusChanged();
    void extendedStatusChanged();
    void deviceNameChanged();
    void lastMessageChanged();

private slots:
    void poll();
    void onMessageReceived(QString sender, QString message);

private:
    void setStatus(Status s);
    void attachToSmsInterface();
    bool validateExistingDevice();
    void discoverNewDevice();

    QString m_deviceId;       // persisted primary device
    QString m_deviceName;
    QString m_deviceStatus;
    QString m_extendedStatus;

    QString m_lastSender;
    QString m_lastMessage;

    QTimer m_pollTimer;
    QDBusInterface *m_smsIface = nullptr;
};
