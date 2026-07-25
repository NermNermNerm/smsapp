#include "devicestatus.h"
#include "messageshandler.h"
#include "dbus.h"


DeviceStatus::DeviceStatus(const QString &specifiedDeviceId, QObject *parent)
    : QObject(parent), m_specifiedDeviceId(specifiedDeviceId)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    // Timer for polling
    auto timer = new QTimer(this);

    // Switch polling interval based on app state
    connect(qApp, &QGuiApplication::applicationStateChanged, this,
            [this, timer](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive) {
                    timer->start(PollIntervalWhenActiveInMs);
                    poll();    // immediate refresh when foregrounded
                } else {
                    timer->start(PollIntervalInBackgroundPoll);
                }
            });

    connect(timer, &QTimer::timeout, this, &DeviceStatus::poll);

    // Listen for device list changes from KDE Connect daemon
    connect(&dbus::daemon(), &org::kde::kdeconnect::daemon::deviceAdded, this, &DeviceStatus::onDeviceListChanged);
    connect(&dbus::daemon(), &org::kde::kdeconnect::daemon::deviceRemoved, this, &DeviceStatus::onDeviceListChanged);

    // Initial poll
    QTimer::singleShot(0, this, &DeviceStatus::poll);
}

DeviceStatus *DeviceStatus::s_instance = nullptr;

void DeviceStatus::poll()
{
    if (m_handler && m_handler->lastDaemonActivityUtc().msecsTo(QDateTime::currentDateTimeUtc()) < PollIntervalWhenActiveInMs) {
        setStatus(Status::DeviceReady);
        return;
    }

    // Check daemon presence
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(dbus::serviceName)) {
        setStatus(Status::DaemonNotRunning);
        return;
    }

    // Check daemon responsiveness
    auto &daemon = dbus::daemon();
    QDBusPendingReply<QStringList> reply = daemon.devices(/* onlyReachable */ false, /* onlyPaired = */ true);
    reply.waitForFinished();
    if (reply.isError()) {
        setStatus(Status::DaemonHung);
        return;
    }

    QStringList knownDeviceIds = reply.value();
    bool otherDevicesListChanged = false;

    if (!m_specifiedDeviceId.isEmpty() && !knownDeviceIds.contains(m_specifiedDeviceId)) {
        setStatus(Status::DeviceMissing);
        return;
    }

    QString previousSessionDeviceId = settings().previousSessionDeviceId();

    // We've established that we can talk to kde, now try and hook a handler if we need to
    if (m_handler == nullptr) {
        if (!m_specifiedDeviceId.isEmpty()) {
            if (knownDeviceIds.contains(m_specifiedDeviceId)) {
                // If we've been told what device to connect to, and the device is still paired, use it regardless of its state.
                setupHandler(m_specifiedDeviceId);
            }
            else {
                // If we've been told what device to connect to, and the device is gone, we're going to a world
                // where the user has to stop the app and get a new plan.
                setStatus(Status::DeviceMissing);
            }
        }
    }

    if (m_handler == nullptr
     && !previousSessionDeviceId.isEmpty()
     && knownDeviceIds.contains(previousSessionDeviceId)
     && dbus::device(previousSessionDeviceId).isReachable()) {
        setupHandler(previousSessionDeviceId);
    }

    // Remove devices that no longer exist
    for (int i = m_otherDevices.size() - 1; i >= 0; --i) {
        const QString &id = m_otherDevices[i].id;
        if (!knownDeviceIds.contains(id)) {
            m_otherDevices.removeAt(i);
            otherDevicesListChanged = true;
        }

        // Our logic should prevent the current device from ever getting into the other devices list.
        Q_ASSERT(m_handler == nullptr || id != m_handler->deviceID());
    }


    // Add or update devices that exist
    for (const QString &id : std::as_const(knownDeviceIds)) {
        auto &dev = dbus::device(id);
        QString name = dev.name();
        if (name.isEmpty()) {
            qWarning() << "KDE Reported the name of" << id << "is an empty string?!";
            continue;
        }

        if (m_handler != nullptr && m_handler->deviceID() == id) {
            setDeviceName(name);
            continue;
        }

        bool hasSms = settings().isDeviceKnownToHaveSms(id) || (dev.isReachable() && dev.hasPlugin("kdeconnect_sms"));
        if (!hasSms) continue;
        settings().setDeviceKnownToHaveSms(id);

        if (m_handler == nullptr && m_specifiedDeviceId.isEmpty()) {
            // If we weren't specifically directed to a device, and the last device we used isn't reachable,
            //  go ahead and use the first reachable sms-capable device.
            // TODO: In the future, it'd be good to ensure that no other instance of the app is
            //   working against this device.
            setupHandler(id);
            settings().setPreviousSessionDeviceId(id);
            setDeviceName(name);
        }
        else {
            // Check if already present
            auto it = std::find_if(m_otherDevices.begin(), m_otherDevices.end(),
                                   [&](const DeviceInfo &d) { return d.id == id; });

            if (it == m_otherDevices.end()) {
                // New device
                m_otherDevices.append(DeviceInfo{id, name});
                otherDevicesListChanged = true;
            } else if (it->name != name) {
                // Name changed
                it->name = name;
                otherDevicesListChanged = true;
            }
        }
    }

    if (otherDevicesListChanged) {
        emit otherDevicesChanged();
    }

    // If we couldn't find any reachable devices, but the last device we used is valid, keep trying to use it.
    if (m_handler == nullptr && !previousSessionDeviceId.isEmpty() && knownDeviceIds.contains(previousSessionDeviceId)) {
        setupHandler(previousSessionDeviceId);
        setDeviceName(dbus::device(previousSessionDeviceId).name());
    }

    if (m_handler == nullptr) {
        // TODO: If another instance of the app is running, perhaps we should just throw that to the front?
        setStatus(Status::NoSmsDevice);
    }

    if (qApp->applicationState() == Qt::ApplicationActive
        && status() == Status::DeviceUnreachable)
    {
        const auto now = QDateTime::currentDateTimeUtc();

        if (!m_lastWakeAttempt.isValid() || m_lastWakeAttempt.secsTo(now) > 30) {
            qDebug() << "Device unreachable; nudging KDE Connect daemon";

            // Sometimes this wakes the phone.  I think.
            dbus::daemon().forceOnNetworkChange();
            m_lastWakeAttempt = now;
        }
    }

    if (m_handler != nullptr) {
        setStatus(dbus::device(m_handler->deviceID()).isReachable() ? Status::DeviceReady : Status::DeviceUnreachable);
    }
}

// DBus signal handler for when a new device might be available.
void DeviceStatus::onDeviceListChanged()
{
    poll();
}

void DeviceStatus::setupHandler(const QString &deviceId)
{
    Q_ASSERT(m_handler == nullptr);
    m_handler = new MessagesHandler(deviceId, this);
    emit handlerChanged();
    setStatus(dbus::device(deviceId).isReachable() ? Status::DeviceReady : Status::DeviceUnreachable);
}

void DeviceStatus::setDeviceName(const QString &deviceName)
{
    if (deviceName == m_deviceName)
        return;

    m_deviceName = deviceName;
    emit deviceNameChanged();
}

void DeviceStatus::setAutoFixDaemon(bool enabled)
{
    if (settings().autoFixDaemon() == enabled)
        return;

    settings().setAutoFixDaemon(enabled);
    emit autoFixDaemonChanged();
}

void DeviceStatus::setStatus(Status status)
{
    if (m_status == status)
        return;

    qDebug() << "Status:" << status;

    m_status = status;
    emit statusChanged();
}

void DeviceStatus::rebootDaemon()
{
    uint pid = QDBusConnection::sessionBus().interface()->servicePid(dbus::serviceName);
    if (pid > 0)
        ::kill(pid, SIGKILL);

    // Hopefully it'll resolve in the next poll.
}
