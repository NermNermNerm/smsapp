#pragma once
#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>

class SmsBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString deviceStatus READ deviceStatus NOTIFY deviceStatusChanged)
    Q_PROPERTY(QString lastSender READ lastSender NOTIFY lastMessageChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    explicit SmsBackend(QObject *parent = nullptr);

    QString deviceStatus() const { return m_deviceStatus; }
    QString lastSender() const { return m_lastSender; }
    QString lastMessage() const { return m_lastMessage; }

signals:
    void deviceStatusChanged();
    void lastMessageChanged();

private slots:
    void onMessageReceived(const QString &sender, const QString &message);

private:
    void checkDeviceStatus();
    void connectToSignals();

    QString m_deviceStatus = "Unknown";
    QString m_lastSender;
    QString m_lastMessage;

    QDBusInterface *m_deviceIface = nullptr;
    QDBusInterface *m_smsIface = nullptr;
};
