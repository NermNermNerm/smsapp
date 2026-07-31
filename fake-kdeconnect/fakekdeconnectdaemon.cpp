// clazy:excludeall=range-loop-detach
#include "fakekdeconnectdaemon.h"
#include "fakekdeconnectdevice.h"
#include "fakedeviceconversationsinterface.h"
#include "fakekdeconnectbattery.h"
#include "commands.h"

#include <QDBusConnection>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "deviceconfig.h"

FakeKdeConnectDaemon::FakeKdeConnectDaemon(QObject *parent)
    : QObject(parent)
{
}

bool FakeKdeConnectDaemon::isValid() const
{
    return true;
}

void FakeKdeConnectDaemon::loadDevices()
{
    m_devices = DeviceConfig::load();
}

bool FakeKdeConnectDaemon::registerOnDBus()
{
    qDBusRegisterMetaType<ConversationAddress>();
    qDBusRegisterMetaType<Attachment>();
    auto bus = QDBusConnection::sessionBus();

    if (!bus.registerService("org.fake.kdeconnect")) {
        qCritical() << "Failed to register DBus service 'org.fake.kdeconnect': "
                    << bus.lastError().message();
        return false;
    }

    // Register the daemon object at /modules/kdeconnect
    if (!bus.registerObject(QStringLiteral("/modules/kdeconnect"),
                            this,
                            QDBusConnection::ExportAllProperties |
                                QDBusConnection::ExportAllSlots |
                                QDBusConnection::ExportAllSignals)) {
        qCritical() << "Failed to register daemon object: "
                    << bus.lastError().message();
        return false;
    }

    // Register each device and its conversations interface
    for (auto& devInfo : std::as_const(m_devices)) {
        if (!registerDevice(devInfo.get())) {
            return false;
        }
    }

    return true;
}

bool FakeKdeConnectDaemon::registerDevice(DeviceConfig *devInfo) {
    auto bus = QDBusConnection::sessionBus();

    const QString devicePath =
        QString("/modules/kdeconnect/devices/%1").arg(devInfo->id);

    // Device object
    auto *deviceObj = new FakeKdeConnectDevice(devInfo, this);
    auto *convObj = new FakeDeviceConversationsInterface(devInfo, deviceObj);
    if (!bus.registerObject(devicePath,
                            deviceObj,
                            QDBusConnection::ExportAllProperties |
                                QDBusConnection::ExportAllSlots |
                                QDBusConnection::ExportAllSignals |
                                QDBusConnection::ExportAdaptors)) {
        qCritical() << "Failed to register device object at " << devicePath
                    << ": " << bus.lastError().message();
        delete deviceObj;
        delete convObj;
        return false;
    }
    m_fakeDevices[devInfo->id] = deviceObj;
    m_fakeConversations[devInfo->id] = convObj;

    const QString batteryPath =
        QString("/modules/kdeconnect/devices/%1/battery").arg(devInfo->id);
    auto *batteryObj = new FakeKdeConnectBattery(this);
    if (!bus.registerObject(batteryPath,
                            batteryObj,
                            QDBusConnection::ExportAllProperties |
                                QDBusConnection::ExportAllSlots |
                                QDBusConnection::ExportAllSignals |
                                QDBusConnection::ExportAdaptors)) {
        qCritical() << "Failed to register battery object at " << batteryPath
                    << ": " << bus.lastError().message();
        delete batteryObj;
        return false;
    }
    m_fakeBattery[devInfo->id] = batteryObj;
    return true;
}

QStringList FakeKdeConnectDaemon::devices()
{
    return devices(false, false);
}

QStringList FakeKdeConnectDaemon::devices(bool onlyReachable)
{
    return devices(onlyReachable, false);
}

QStringList FakeKdeConnectDaemon::devices(bool onlyReachable, bool onlyPaired)
{
    qInfo() << "daemon::devices queried onlyreachable" << onlyReachable << "onlyPaired" << onlyPaired;
    QStringList ids;
    for (const auto &dev : m_devices) {
        if (!onlyReachable || dev->reachable) {
            ids << dev.get()->id;
        }
    }
    return ids;
}

void FakeKdeConnectDaemon::reset()
{
    qInfo() << "Resetting for new client";
    for (auto *conv : m_fakeConversations)
        conv->reset();
}

void FakeKdeConnectDaemon::startInteractive()
{
    // Delegate to the commands module which will set up the notifier and callbacks.
    m_selectedIndex = 0;
    Commands::startInteractiveShell(this);
}

bool FakeKdeConnectDaemon::checkSelected() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_devices.size()) {
        qWarning() << "No device selected";
        return false;
    }
    return true;
}

int FakeKdeConnectDaemon::newDevice(const DeviceConfig &config)
{
    auto *myCopy = new DeviceConfig(config);
    m_devices.emplace_back(myCopy);
    myCopy->save();
    registerDevice(myCopy);
    emit deviceAdded(config.id);
    emit deviceListChanged();
    return m_devices.size()-1;
}

void FakeKdeConnectDaemon::removeDevice(int deviceIndex)
{
    auto *cfg = m_devices[deviceIndex].get();
    const QString id = cfg->id;

    cfg->remove();

    auto bus = QDBusConnection::sessionBus();

    const QString devicePath =
        QString("/modules/kdeconnect/devices/%1").arg(id);
    const QString batteryPath =
        QString("/modules/kdeconnect/devices/%1/battery").arg(id);

    // Unregister DBus objects first
    bus.unregisterObject(devicePath);
    bus.unregisterObject(batteryPath);

    // Delete QObjects (convObj is child of deviceObj)
    delete m_fakeDevices[id];
    delete m_fakeBattery[id];

    // Remove from maps
    m_fakeDevices.remove(id);
    m_fakeConversations.remove(id);   // no delete
    m_fakeBattery.remove(id);

    // Remove from vector
    auto it = m_devices.begin();
    std::advance(it, deviceIndex);
    m_devices.erase(it);

    emit deviceRemoved(id);
    emit deviceListChanged();
}

int FakeKdeConnectDaemon::restoreDevice(const QString &id)
{
    auto *myCopy = DeviceConfig::restore(id);
    if (myCopy == nullptr)
        return -1;

    m_devices.emplace_back(myCopy);
    const int index = m_devices.size() - 1;

    registerDevice(myCopy);

    emit deviceAdded(id);
    emit deviceListChanged();

    return index;
}


