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

        const QString devicePath =
            QString("/modules/kdeconnect/devices/%1").arg(devInfo->id);

        // Device object
        auto *deviceObj = new FakeKdeConnectDevice(devInfo.get(), this);
        auto *convObj = new FakeDeviceConversationsInterface(devInfo.get(), deviceObj);
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
    }

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
