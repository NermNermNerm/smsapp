#include "devicestatus.h"
#include "messageshandler.h"
#include "dbus.h"

#include <QTimer>
#include <QGuiApplication>
#include <QDBusConnection>
#include <QDBusReply>
#include <signal.h>

DeviceStatus::DeviceStatus(QObject *parent)
    : QObject(parent)
{
    // Timer for polling
    auto timer = new QTimer(this);
    timer->start(PollIntervalWhenActiveInMs);

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

    // Listen for device list changes from KDE Connect daemon
    QDBusConnection::sessionBus().connect(
        dbus::serviceName,
        dbus::daemonPath,
        dbus::daemon().staticInterfaceName(),
        "deviceAdded",
        this,
        SLOT(onDeviceListChanged())
        );

    QDBusConnection::sessionBus().connect(
        dbus::serviceName,
        dbus::daemonPath,
        dbus::daemon().staticInterfaceName(),
        "deviceRemoved",
        this,
        SLOT(onDeviceListChanged())
        );

    // Initial poll
    poll();
}

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
    if (!tryRefreshDeviceList())
    {
        setStatus(Status::DaemonHung);
        return;
    }

    // Ensure we set the status before exiting.
    if (!m_handler && preferredDevice().isEmpty()) {
        setStatus(Status::NoSmsDevice);
    }
    else if (m_handler && dbus::device(m_handler->deviceID()).isReachable()) {
        setStatus(Status::DeviceReady);
    }
    else {
        setStatus(Status::DeviceUnreachable);
    }
}

// DBus signal handler for when a new device might be available.
void DeviceStatus::onDeviceListChanged()
{
    tryRefreshDeviceList();
}

// Refresh list of devices that support sms
bool DeviceStatus::tryRefreshDeviceList()
{
    auto &daemon = dbus::daemon();
    QDBusPendingReply<QStringList> reply = daemon.devices(true, true);
    reply.waitForFinished();
    if (reply.isError())
        return false;

    QStringList ids = reply.value();
    bool changed = false;

    // Remove devices that no longer exist
    for (int i = m_validDevices.size() - 1; i >= 0; --i) {
        const QString &id = m_validDevices[i].id;
        if (!ids.contains(id)) {
            m_validDevices.removeAt(i);
            changed = true;
        }
    }

    // Add or update devices that exist
    for (const QString &id : std::as_const(ids)) {

        auto &dev = dbus::device(id);

        QString name = dev.name();
        if (name.isEmpty())
            continue;

        QStringList plugins = dev.loadedPlugins();
        if (!plugins.contains("sms"))
            continue;

        // Check if already present
        auto it = std::find_if(m_validDevices.begin(), m_validDevices.end(),
                               [&](const DeviceInfo &d) { return d.id == id; });

        if (it == m_validDevices.end()) {
            // New device
            m_validDevices.append(DeviceInfo{id, name});
            changed = true;
        } else if (it->name != name) {
            // Name changed
            it->name = name;
            changed = true;
        }
    }

    if (changed) {
        emit validDevicesChanged();
        trySetupPreferredDevice();
    }

    return true;
}

// If preferredDevice() is empty or pointing to a device that doesn't exist, this will
// set it to the first valid and reachable device.  Otherwise it does nothing.
void DeviceStatus::trySetupPreferredDevice()
{
    if (m_validDevices.empty())
    {
        // We have no devices; we'll figure we got here because, whatever is wrong, we won't
        // be able to provide a better story here.
        return;
    }

    // Find the preferred device in m_validDevices, if it's there.
    auto preferredDeviceIt = std::find_if(m_validDevices.begin(), m_validDevices.end(),
                           [&](const DeviceInfo &d) {
                               return d.id == preferredDevice();
                           });

    // If the old preferred device is no longer there or it was never set,
    // we'll attach to the first device that is valid and reachable.
    if (preferredDeviceIt == m_validDevices.end()) {
        auto reachableDeviceIt = std::find_if(m_validDevices.begin(), m_validDevices.end(),
                                              [&](const DeviceInfo &dev) {
                                                  return dbus::device(dev.id).isReachable();
                                              });

        if (reachableDeviceIt != m_validDevices.end())
        {
            setPreferredDevice(reachableDeviceIt->id);
            return;
        }

        // no reachable devices to switch to...
    }
}

void DeviceStatus::updateHandler()
{
    if (!m_handler && preferredDevice().isEmpty())
        return;

    if (m_handler && m_handler->deviceID() == preferredDevice())
        return;

    if (m_handler)
    {
        m_handler->deleteLater();
        m_handler = nullptr;
    }

    if (!preferredDevice().isEmpty()) {
        m_handler = new MessagesHandler(preferredDevice(), this);
        setStatus(dbus::device(preferredDevice()).isReachable() ? Status::DeviceReady : Status::DeviceUnreachable);
    }
    emit handlerChanged();
}

void DeviceStatus::setPreferredDevice(const QString &deviceID)
{
    if (deviceID == settings().preferredDeviceId())
        return;

    settings().setPreferredDeviceId(deviceID);
    emit preferredDeviceChanged();

    updateHandler();
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
