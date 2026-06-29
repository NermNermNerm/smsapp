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
