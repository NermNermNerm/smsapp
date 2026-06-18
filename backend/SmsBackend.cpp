#include "SmsBackend.h"
#include "kdeconnect_proxy.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include "kdeconnect_interfaces/conversationmessage.h"

static const QString orgKdeConnect = "org.kde.kdeconnect"; // "org.fake.kdeconnect";

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
SmsBackend::SmsBackend(QObject *parent)
    : QObject(parent), m_daemon(orgKdeConnect, "/modules/kdeconnect", QDBusConnection::sessionBus())
{
    qDebug() << "In constructor";
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
    if (!m_device->isValid()) {
        setStatus(Status::DeviceRemoved);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return false;
    }

    if (!m_device->isReachable()) {
        setStatus(Status::DeviceUnreachable);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return false;
    }

    auto name = m_device->name();
    if (name != m_deviceName)
    {
        m_deviceName = name;
        emit deviceNameChanged();
    }

    // SMS plugin?
    QDBusReply<bool> hasSms = m_device->hasPlugin("kdeconnect_sms");
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

    setStatus(Status::Ok);
    QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
    return true;
}

// ------------------------------------------------------------
// Discover new device (only when no primary device)
// ------------------------------------------------------------
void SmsBackend::discoverNewDevice()
{
    // TODO: m_daemon.isValid seems to always be true, even if the daemon isn't running.
    //   What we need to od is look at the error codes from m_daemon.devices(), rather than
    //   trusting it to be working.

    if (!m_daemon.isValid()) {
        setStatus(Status::DaemonUnavailable);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return;
    }

    for (const QString &id : m_daemon.devices().value()) {
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

        // Create all interfaces
        m_device = new org::kde::kdeconnect::device(
            orgKdeConnect,
            "/modules/kdeconnect/devices/" + id,
            QDBusConnection::sessionBus(),
            this
            );

        m_conversations = new org::kde::kdeconnect::conversations(
            orgKdeConnect,
            "/modules/kdeconnect/devices/" + id,
            QDBusConnection::sessionBus(),
            this
            );

        m_deviceId = id;
        m_deviceName = dev.name();
        emit deviceNameChanged();

        attachToSmsInterface();
        setStatus(Status::Ok);
        QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
        return;
    }

    setStatus(Status::NoPrimaryDevice);
    QTimer::singleShot(SmsBackend::PollIntervalInMs, this, &SmsBackend::poll);
}

// ------------------------------------------------------------
// Attach to SMS interface
// ------------------------------------------------------------
void SmsBackend::attachToSmsInterface()
{
    // Handle the "conversationLoaded" signal
    QObject::connect(m_conversations, &org::kde::kdeconnect::conversations::conversationLoaded,
            this,
            [this](qint64 threadId, quint64 messageCount) {
                m_conversations->requestConversation(threadId, 1, 1);
                // Your handler logic here
                qDebug() << "Thread" << threadId << "has" << messageCount << "messages";
            });

    QObject::connect(m_conversations, &org::kde::kdeconnect::conversations::conversationLoaded,
                     this,
                     [this](qint64 threadId, quint64 messageCount) {
                         m_conversations->requestConversation(threadId, 1, 1);
                         // Your handler logic here
                         qDebug() << "Thread" << threadId << "has" << messageCount << "messages";
                     });
    QObject::connect(m_conversations, &org::kde::kdeconnect::conversations::conversationCreated,
                     this, &SmsBackend::handleConversationCreated);
    QObject::connect(m_conversations, &org::kde::kdeconnect::conversations::conversationUpdated,
                     this, &SmsBackend::handleConversationUpdate);

    m_conversations->requestAllConversationThreads();
    // TODO
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


void SmsBackend::handleConversationUpdate(const QDBusVariant &msg)
{
    int lengthBefore = m_conversationList.rowCount();
    ConversationMessage message = ConversationMessage::fromDBus(msg);
    m_conversationList.addOrUpdateConversation(message);
}

void SmsBackend::handleConversationCreated(const QDBusVariant &msg)
{
    int lengthBefore = m_conversationList.rowCount();
    ConversationMessage message = ConversationMessage::fromDBus(msg);
    m_conversationList.addOrUpdateConversation(message);
}

