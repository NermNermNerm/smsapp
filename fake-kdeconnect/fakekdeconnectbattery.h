#pragma once

#include <QDBusAbstractAdaptor>
#include <QObject>
#include <QTimer>
#include <algorithm>

class FakeKdeConnectBattery : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.battery")

public:
    static inline const char *staticInterfaceName()
    { return "org.kde.kdeconnect.device.battery"; }

    explicit FakeKdeConnectBattery(QObject *parent = nullptr);

    Q_PROPERTY(int charge READ charge)
    inline int charge() const { return std::clamp((int)m_level, 0, 100); }

    Q_PROPERTY(bool isCharging READ isCharging)
    inline bool isCharging() const { return m_changePerMinute > 0; }

public Q_SLOTS:
    // no methods; just properties

Q_SIGNALS:
    void refreshed(bool isCharging, int charge);

public:
    void setLevel(int level) {
        if ( level != charge() ) {
            m_level = level;
            emit refreshed(isCharging(), charge());
        }
    }

    void setChargeRate(double changePerMinute) {
        m_changePerMinute = changePerMinute;
    }

private:
    double m_level = 50;
    double m_changePerMinute = 0;

    QTimer m_changeTimer;
};
