// clazy:excludeall=qcolor-from-literal
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

QString Settings::previousSessionDeviceId() const
{
    return m_settings.value("previousSessionDeviceId").toString();
}

void Settings::setPreviousSessionDeviceId(const QString &id)
{
    m_settings.setValue("previousSessionDeviceId", id);
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

static const QVector<QColor> darkBackgroundColors = {
    QColor("#303F9F"), // dark indigo
    QColor("#455A64"), // dark blue-grey
    QColor("#5D4037"), // dark brown
    QColor("#37474F")  // darker blue-grey
};

QColor Settings::getColorForDevice(const QString &deviceID)
{
    const QString key = QStringLiteral("deviceColors/%1").arg(deviceID);
    QVariant stored = m_settings.value(key);
    if (stored.isValid()) {
        return stored.value<QColor>();
    }

    QSet<QString> used;
    for (const QString &k : m_settings.allKeys()) {
        if (!k.startsWith("deviceColors/"))
            continue;

        QVariant v = m_settings.value(k);
        if (v.isValid()) {
            used.insert(v.value<QColor>().name());
        }
    }

    QColor assigned;

    // First try to assign an unused palette color.
    for (const QColor &c : darkBackgroundColors) {
        if (!used.contains(c.name())) {
            assigned = c;
            break;
        }
    }

    // If all palette colors are used, choose a pseudo-random fallback.
    if (!assigned.isValid()) {
        // Hash last character of device ID.
        ushort code = 0;
        if (!deviceID.isEmpty()) {
            QChar last = deviceID.at(deviceID.size() - 1);
            code = last.unicode();
        }

        int index = code % darkBackgroundColors.size();
        assigned = darkBackgroundColors.at(index);
    }

    m_settings.setValue(key, assigned);
    return assigned;
}

void Settings::setColorForDevice(const QString &deviceID, QColor color)
{
    const QString key = QStringLiteral("deviceColors/%1").arg(deviceID);
    m_settings.setValue(key, color);
}
