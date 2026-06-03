#include "SmsBackend.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
SmsBackend::SmsBackend(QObject *parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &SmsBackend::poll);
    m_pollTimer.start(2000);

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
             ->isServiceRegistered("org.kde.kdeconnect")) {
        setStatus(Status::DaemonUnavailable);
        return;
    }

    // 2. If we have a primary device, validate it
    if (!m_deviceId.isEmpty()) {
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
    QDBusInterface dev(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices/" + m_deviceId,
        "org.kde.kdeconnect.device",
        QDBusConnection::sessionBus()
        );


    QVariant reachableVar = dev.property("isReachable");

    // Device removed?
    if (!reachableVar.isValid()) {
        // This is probably an incorrect reading.  I think this is only true if the interface has changed,
        //  thus it's more of a "there's a bug in this program" indicator.
        setStatus(Status::DeviceRemoved);
        return false;
    }

    // Unreachable?
    if (!reachableVar.toBool()) {
        setStatus(Status::DeviceUnreachable);
        return false;
    }

    // SMS plugin?
    QDBusReply<bool> hasSms = dev.call("hasPlugin", "kdeconnect_sms");
    if (!hasSms.isValid() || !hasSms.value()) {
        setStatus(Status::SmsPluginUnavailable);
        return false;
    }

    // SMS interface valid?
    QDBusInterface smsIface(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices/" + m_deviceId + "/sms",
        "org.kde.kdeconnect.device.sms",
        QDBusConnection::sessionBus()
        );

    if (!smsIface.isValid()) {
        setStatus(Status::SmsInterfaceInvalid);
        return false;
    }

    // Update name if needed.
    QVariant nameVar = dev.property("name");
    QString deviceName = nameVar.isValid() ? nameVar.toString()
                                           : QStringLiteral("Unknown device");

    // All good
    if (!m_smsIface)
        attachToSmsInterface();

    setStatus(Status::Ok);
    return true;
}

// ------------------------------------------------------------
// Discover new device (only when no primary device)
// ------------------------------------------------------------
void SmsBackend::discoverNewDevice()
{
    QDBusInterface daemon(
        "org.kde.kdeconnect",
        "/modules/kdeconnect",
        "org.kde.kdeconnect.daemon",
        QDBusConnection::sessionBus()
        );

    QDBusReply<QStringList> reply = daemon.call("devices");
    if (!reply.isValid()) {
        setStatus(Status::DaemonUnavailable);
        return;
    }

    for (const QString &id : reply.value()) {
        QDBusInterface dev(
            "org.kde.kdeconnect",
            "/modules/kdeconnect/devices/" + id,
            "org.kde.kdeconnect.device",
            QDBusConnection::sessionBus()
            );

        QVariant reachableVar = dev.property("isReachable");
        bool reachable = reachableVar.isValid() && reachableVar.toBool();

        QDBusReply<bool> hasSms = dev.call("hasPlugin", "kdeconnect_sms");
        QVariant nameVar = dev.property("name");
        QString deviceName = nameVar.isValid() ? nameVar.toString()
                                               : QStringLiteral("Unknown device");

        if (reachable &&
            hasSms.isValid() && hasSms.value()) {

            m_deviceId = id;
            m_deviceName = deviceName;
            emit deviceNameChanged();

            attachToSmsInterface();
            setStatus(Status::Ok);
            return;
        }
    }

    setStatus(Status::NoPrimaryDevice);
}

// ------------------------------------------------------------
// Attach to SMS interface
// ------------------------------------------------------------
void SmsBackend::attachToSmsInterface()
{
    delete m_smsIface;

    m_smsIface = new QDBusInterface(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices/" + m_deviceId + "/sms",
        "org.kde.kdeconnect.device.sms",
        QDBusConnection::sessionBus(),
        this
        );

    QDBusConnection::sessionBus().connect(
        "org.kde.kdeconnect",
        "/modules/kdeconnect/devices/" + m_deviceId + "/sms",
        "org.kde.kdeconnect.device.sms",
        "messageReceived",
        this,
        SLOT(onMessageReceived(QString, QString))
        );
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

    m_deviceStatus = shortMap.value(s);
    m_extendedStatus = longMap.value(s);

    emit deviceStatusChanged();
    emit extendedStatusChanged();
}
