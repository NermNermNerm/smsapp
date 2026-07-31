#include "instancemanager.h"

static QString serviceNameFor(const QString &deviceId)
{
    return QStringLiteral("org.nermnermnerm.smsapp.instance.%1").arg(deviceId);
}

void InstanceManager::claimOrExit(const QString &deviceId)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    const QString serviceName = serviceNameFor(deviceId);

    // If another instance is already managing this device
    if (bus.interface()->isServiceRegistered(serviceName)) {

        // Ask it to raise its window
        QDBusMessage msg = QDBusMessage::createMethodCall(
            serviceName,
            "/Window",
            "",
            "RaiseWindow"
            );

        bus.call(msg, QDBus::NoBlock);

        qInfo() << "Another process is already taking care of this phone.";
        QCoreApplication::exit(0);
        return;
    }

    // Otherwise, claim the device by registering the DBus service
    if (!bus.registerService(serviceName)) {
        // Race condition or DBus failure — safest fallback is to exit
        qInfo() << "Another process is already taking care of this phone.";
        QCoreApplication::exit(0);
        return;
    }

    // Export the RaiseWindow() API
    bus.registerObject("/Window",
                       new InstanceManager(),
                       QDBusConnection::ExportAllSlots);
}