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
    qInfo() << "device::hasPlugin " << plugin << " called on " << m_info->name << "-- returning" << m_info->reachable;
    // If the device is unreachable, the real daemon will return false for any 'hasPlugin' query.
    // We return true for all reachable devices just because we just don't have a need to simulate
    // non-text-enabled devices.
    return m_info->reachable;
}

void FakeKdeConnectDevice::setReachable(bool isReachable)
{
    if (m_info->reachable != isReachable) {
        m_info->reachable = isReachable;
        emit reachableChanged(isReachable);
        emit pluginsChanged();
        m_info->save();
    }
}
