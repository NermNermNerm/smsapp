#include "settings.h"
#include "dbus.h"

Settings& Settings::instance()
{
    static Settings s(dbus::isUsingFakeDBus());
    return s;
}

Settings::Settings(bool isUsingFakeDBus)
    : m_settings("NermNermNerm", isUsingFakeDBus ? "SmsAppFake" : "SmsApp")
{
}

QString Settings::preferredDeviceId() const
{
    return m_settings.value("preferredDeviceId").toString();
}

void Settings::setPreferredDeviceId(const QString &id)
{
    m_settings.setValue("preferredDeviceId", id);
}

bool Settings::autoFixDaemon() const
{
    return m_settings.value("autoFixDaemon", false).toBool();
}

void Settings::setAutoFixDaemon(bool enabled)
{
    m_settings.setValue("autoFixDaemon", enabled);
}

bool Settings::isDeviceKnownToHaveSms(const QString &deviceID) const
{
    const QStringList list = m_settings.value("devicesWithSms").toStringList();
    return list.contains(deviceID);
}

void Settings::setDeviceKnownToHaveSms(const QString &deviceID)
{
    QStringList list = m_settings.value("devicesWithSms").toStringList();

    if (!list.contains(deviceID)) {
        list.append(deviceID);
        m_settings.setValue("devicesWithSms", list);
    }
}
