#include "instancemanager.h"

static QString serviceNameFor(const QString &deviceId)
{
    return QStringLiteral("org.nermnermnerm.smsapp.instance._%1").arg(deviceId);
}

bool InstanceManager::claimOrExit(const QString &deviceId)
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
            "activateApp"
            );

        bus.call(msg, QDBus::NoBlock);

        qInfo() << "Another process is already taking care of this phone.";
        QCoreApplication::exit(0);
        return false;
    }

    // Otherwise, claim the device by registering the DBus service
    if (!bus.registerService(serviceName)) {
        // Race condition or DBus failure — safest fallback is to exit
        qInfo() << "Failed to register the service: " << bus.lastError().name()
                << bus.lastError().message();
        QCoreApplication::exit(0);
        return false;
    }

    // Export the RaiseWindow() API
    bus.registerObject("/Window",
                       new InstanceManager(),
                       QDBusConnection::ExportAllSlots);

    return true;
}

static QWindow *getMainWindow()
{
    auto windows = QGuiApplication::topLevelWindows();
    if (windows.isEmpty())
        windows = QGuiApplication::allWindows();

    if (windows.isEmpty())
        return nullptr;

    return windows.first();
}

void InstanceManager::activateApp()
{
    auto *win = getMainWindow();
    Q_ASSERT(win != nullptr);
    if (win == nullptr)
        return;

    // 1. Un-minimize
    if (win->windowState() & Qt::WindowMinimized) {
        win->setWindowState((Qt::WindowState)(win->windowState() & ~Qt::WindowMinimized));
    }

    win->show();

    // 2. Temporarily set StaysOnTop to force the Window Manager to raise it
    win->setFlag(Qt::WindowStaysOnTopHint, true);
    win->raise();
    win->requestActivate();

    // 3. Remove the StaysOnTop flag shortly after so it behaves normally again
    QTimer::singleShot(200, win, [win]() {
        win->setFlag(Qt::WindowStaysOnTopHint, false);
        win->raise();
        win->requestActivate();
    });
}