#include "SmsBackend.h"
#include <QDBusConnection>
#include <QDBusReply>
#include <QTimer>

SmsBackend::SmsBackend(QObject *parent)
    : QObject(parent)
{
    // KDE Connect daemon device interface
    m_deviceIface = new QDBusInterface(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices",
        "org.kde.kdeconnect.daemon",
        QDBusConnection::sessionBus(),
        this
    );

    // SMS plugin interface (device ID "phone" is placeholder)
    m_smsIface = new QDBusInterface(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices/phone/sms",
        "org.kde.kdeconnect.device.sms",
        QDBusConnection::sessionBus(),
        this
    );

    // Poll device status every 2 seconds
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SmsBackend::checkDeviceStatus);
    timer->start(2000);

    connectToSignals();
}

void SmsBackend::checkDeviceStatus()
{
    if (!m_deviceIface->isValid()) {
        m_deviceStatus = "No device";
        emit deviceStatusChanged();
        return;
    }

    QDBusReply<QStringList> reply = m_deviceIface->call("devices");
    if (!reply.isValid() || reply.value().isEmpty()) {
        m_deviceStatus = "Disconnected";
    } else {
        m_deviceStatus = "Connected";
    }

    emit deviceStatusChanged();
}

void SmsBackend::connectToSignals()
{
    // Listen for incoming SMS messages
    QDBusConnection::sessionBus().connect(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices/29197ada4a9e4c09b88de431c1d4471d/sms",
        "org.kde.kdeconnect.device.sms",
        "messageReceived",
        this,
        SLOT(onMessageReceived(QString, QString))
    );
}

void SmsBackend::onMessageReceived(const QString &sender, const QString &message)
{
    m_lastSender = sender;
    m_lastMessage = message;
    emit lastMessageChanged();
}
