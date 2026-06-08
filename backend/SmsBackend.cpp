#include "SmsBackend.h"
#include "kdeconnect_proxy.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>

static const QString orgKdeConnect = "org.fake.kdeconnect"; // "org.kde.kdeconnect";

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
SmsBackend::SmsBackend(QObject *parent)
    : QObject(parent)
{
    QTimer::singleShot(0, this, &SmsBackend::poll);

    // Load persisted deviceId here (TODO: settings integration)
    // m_deviceId = loadFromSettings();
}

// ------------------------------------------------------------
// Poll loop: 2-second reconciliation
// ------------------------------------------------------------
void SmsBackend::poll()
{
    // 1. KDE Connect daemon available?
    if (!QDBusConnection::sessionBus().interface()
             ->isServiceRegistered(orgKdeConnect)) {
        setStatus(Status::DaemonUnavailable);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return;
    }

    // 2. If we have a primary device, validate it
    if (m_deviceId.isEmpty()) {
        discoverNewDevice();
    }
    else
    {
        validateExistingDevice();
    }
}

// ------------------------------------------------------------
// Validate existing device
// ------------------------------------------------------------
bool SmsBackend::validateExistingDevice()
{
    org::kde::kdeconnect::device dev(
        orgKdeConnect,
        "/modules/kdeconnect/devices/" + m_deviceId,
        QDBusConnection::sessionBus(),
        this
        );

    if (!dev.isValid()) {
        setStatus(Status::DeviceRemoved);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return false;
    }

    if (!dev.isReachable()) {
        setStatus(Status::DeviceUnreachable);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return false;
    }

    auto name = dev.name();
    if (name != m_deviceName)
    {
        m_deviceName = name;
        emit deviceNameChanged();
    }

    // SMS plugin?
    QDBusReply<bool> hasSms = dev.hasPlugin("kdeconnect_sms");
    if (!hasSms.isValid() || !hasSms.value()) {
        setStatus(Status::SmsPluginUnavailable);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return false;
    }

    org::kde::kdeconnect::sms sms(
        orgKdeConnect,
        "/modules/kdeconnect/devices/" + m_deviceId + "/sms",
        QDBusConnection::sessionBus(),
        this
        );

    if (!sms.isValid())
    {
        setStatus(Status::SmsInterfaceInvalid);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return false;
    }

    attachToSmsInterface();

    setStatus(Status::Ok);
    QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
    return true;
}

// ------------------------------------------------------------
// Discover new device (only when no primary device)
// ------------------------------------------------------------
void SmsBackend::discoverNewDevice()
{
    org::kde::kdeconnect::daemon daemon(
        orgKdeConnect,
        "/modules/kdeconnect",
        QDBusConnection::sessionBus()
        );

    if (!daemon.isValid()) {
        setStatus(Status::DaemonUnavailable);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return;
    }

    for (const QString &id : daemon.devices().value()) {
        org::kde::kdeconnect::device dev(
            orgKdeConnect,
            "/modules/kdeconnect/devices/" + id,
            QDBusConnection::sessionBus(),
            this
            );

        // TODO: create some kind of status line for each failed device.
        if (!dev.isValid()) {
            continue;
        }

        if (!dev.isReachable()) {
            continue;
        }

        if (!dev.hasPlugin("kdeconnect_sms")) {
            continue;
        }

        m_deviceId = id;
        m_deviceName = dev.name();
        emit deviceNameChanged();

        attachToSmsInterface();
        setStatus(Status::Ok);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
    }

    setStatus(Status::NoPrimaryDevice);
    QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
}

// ------------------------------------------------------------
// Attach to SMS interface
// ------------------------------------------------------------
void SmsBackend::attachToSmsInterface()
{
    // TODO
}

// ------------------------------------------------------------
// Incoming message
// ------------------------------------------------------------
void SmsBackend::onMessageReceived(QString sender, QString message)
{
    m_lastSender = sender;
    m_lastMessage = message;
    emit lastMessageChanged();
}

// ------------------------------------------------------------
// Set status + extended status
// ------------------------------------------------------------
void SmsBackend::setStatus(Status s)
{
    static const QMap<Status, QString> shortMap = {
        { Status::Ok,                   "Connected" },
        { Status::DaemonUnavailable,    "KDE Connect not running" },
        { Status::DeviceUnreachable,    "Phone unreachable" },
        { Status::SmsPluginUnavailable, "SMS plugin unavailable" },
        { Status::SmsInterfaceInvalid,  "SMS service not ready" },
        { Status::DeviceRemoved,        "Primary device removed" },
        { Status::NoPrimaryDevice,      "No primary device" }
    };

    static const QMap<Status, QString> longMap = {
        { Status::Ok,
         "Your phone is connected and ready." },

        { Status::DaemonUnavailable,
         "KDE Connect is not running. Start KDE Connect or enable it at login so this app can talk to your phone." },

        { Status::DeviceUnreachable,
         "Your phone is paired but unreachable. Wake it up, ensure it's on Wi‑Fi, or bring it back into range. "
         "If the battery is low, plug it in — KDE Connect often sleeps when power is tight." },

        { Status::SmsPluginUnavailable,
         "Your phone is connected, but the SMS feature is turned off. "
         "Open KDE Connect on your phone and enable the SMS plugin." },

        { Status::SmsInterfaceInvalid,
         "Your phone is connected, but the SMS service isn't ready yet. "
         "This usually fixes itself in a moment." },

        { Status::DeviceRemoved,
         "Your primary phone was unpaired or removed from KDE Connect. "
         "Click 'Forget Device' to select a new phone." },

        { Status::NoPrimaryDevice,
         "No primary phone is selected. Pair your phone with KDE Connect, then choose it as your SMS device." }
    };

    m_rawDeviceStatus = s;
    m_deviceStatus = shortMap.value(s);
    m_extendedStatus = longMap.value(s);

    emit deviceStatusChanged();
    emit extendedStatusChanged();
}
