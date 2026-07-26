#include "fakekdeconnectbattery.h"
#include <QTimer>

FakeKdeConnectBattery::FakeKdeConnectBattery(QObject *parent)
    : QDBusAbstractAdaptor{parent}, m_changeTimer(this)
{
    m_changeTimer.setInterval(15'000);
    connect(&m_changeTimer, &QTimer::timeout, this, [this]() {
        int oldCharge = charge();
        m_level += m_changePerMinute/4.0;
        if (charge() != oldCharge) {
            emit refreshed(m_changePerMinute > 0, charge());
        }
    });
    m_changeTimer.start();
}
