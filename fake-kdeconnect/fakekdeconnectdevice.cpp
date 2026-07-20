#include "fakekdeconnectdevice.h"
#include "deviceconfig.h"   // for DeviceConfig

FakeKdeConnectDevice::FakeKdeConnectDevice(DeviceConfig *info, QObject *parent)
    : QObject(parent), m_info(info)
{}

bool FakeKdeConnectDevice::isReachable() const {
    qInfo() << "device::isReachable called on " << m_info->name << "-- returning " << m_info->reachable;
    return m_info->reachable;
}

QString FakeKdeConnectDevice::name() const {
    return m_info->name;
}

bool FakeKdeConnectDevice::hasPlugin(const QString &plugin) const {
    qInfo() << "device::hasPlugin " << plugin << " called on " << m_info->name << "-- returning true";
    return true;
}
