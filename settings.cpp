#include "settings.h"

Settings& Settings::instance()
{
    static Settings s;
    return s;
}

Settings::Settings()
    : m_settings("NermNermNerm", "SmsApp")
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
