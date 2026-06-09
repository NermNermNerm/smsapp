#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QPainter>
#include <QIcon>
#include "backend/SmsBackend.h"

static QQmlApplicationEngine *global_engine = nullptr;
static QQuickWindow *my_qml_window = nullptr;

/*  This function contains hacks that fail to show status on the launcher panel.

void updateLinuxBadgeCount(int count) {
// This part sends a message over KDE -- this might work on GNOME

    // Must match your app's actual launcher filename
    QString launcherId = "application://smsapp.desktop";

    QVariantMap properties;
    properties.insert("count", count);                // The number to display
    properties.insert("count-visible", count > 0);    // Hide badge entirely if 0

    // Create the D-Bus signal targeted at the system launcher entry
    QDBusMessage signal = QDBusMessage::createSignal(
        "/com/canonical/unity/launcherentry/1", // Apparently this string doesn't even matter (??!)
        "com.canonical.Unity.LauncherEntry",
        "Update"
        );

    signal << launcherId;
    signal << properties;

    // Send it to the Linux desktop environment
    QDBusConnection::sessionBus().send(signal);

// This attempts to change the icon dynamically.  I don't believe this works ever.

    QPixmap basePixmap("/home/steve/repos/smsapp/reddoticon.png");
    if (basePixmap.isNull()) return; // Safety check

    // Scale it nicely so we can paint on it reliably
    basePixmap = basePixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter painter(&basePixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw the red badge circle in the top-right corner
    painter.setBrush(QColor("#ef4444")); // Smooth red
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(34, 2, 28, 28); // Position (X, Y, Width, Height)

    // Draw the white text number inside the circle
    painter.setPen(QColor(Qt::white));
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(count > 9 ? 14 : 18); // Shrink font slightly for double digits
    painter.setFont(font);

    // Center the text inside the red circle
    painter.drawText(QRect(34, 2, 28, 28), Qt::AlignCenter, QString::number(count));
    painter.end();
    QIcon newIcon(basePixmap);
    QGuiApplication::setWindowIcon(newIcon);

    // 2. Explicitly force it onto the active window instances
    QList<QWindow*> allWindows = QGuiApplication::topLevelWindows();
    for (QWindow *window : allWindows) {
        window->setIcon(newIcon);
        // window->requestActivate();
    }


// This is a real thing that works - sending a notification when a text message arrives.
// Maybe we'll do this, but not here.

    // 1. Target the standard Linux Freedesktop Notification service
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.Notifications",		// Service
        "/org/freedesktop/Notifications",	// Object Path
        "org.freedesktop.Notifications",	// Interface
        "Notify"					// Method
        );

    // 2. Set up the notification parameters
    QString appName = "smsapp";            // Crucial: Must match your .desktop filename prefix
    uint replacesId = 0;                   // 0 means create a brand new notification
    QString appIcon = "smsapp";            // Tells Cinnamon which icon asset to use
    QString summary = "SMS Mirror";
    QString body    = "Howyadoin";

    QStringList actions;                   // Empty list (no action buttons)
    QVariantMap hints;                     // Extra metadata hints
    hints.insert("desktop-entry", "smsapp"); // Explicitly tie this popup to smsapp.desktop

    int timeout = 5000;                    // Clear popup after 5 seconds (-1 for system default)

    // 3. Stuff the arguments into the D-Bus message payload
    msg << appName << replacesId << appIcon << summary << body << actions << hints << timeout;

    // 4. Fire it off onto the session bus
    QDBusConnection::sessionBus().send(msg);
}
*/

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Force the Linux Window Manager to register this exact string as the WM_CLASS
    app.setApplicationName("appsmsapp");
    app.setDesktopFileName("smsapp");

    // For setting the icon and all that, this relies on the existance of:
    //  ~/.local/share/applications/smsapp.desktop.  The "smsapp" on the
    //  setDesktopFileName corresponds to the 'smsapp' in the file.
    //  The contents of the file are:
/*
[Desktop Entry]
Type=Application
Name=SMS Mirror
Exec=/home/steve/repos/smsapp/build/Desktop_Qt_6_11_1-Debug/appsmsapp
Icon=/home/steve/repos/smsapp/defaulticon.png
Terminal=false
 */
    QGuiApplication::setDesktopFileName("smsapp");

    qmlRegisterType<SmsBackend>("Sms", 1, 0, "SmsBackend");
    SmsBackend backend;

    QQmlApplicationEngine *global_engine = new QQmlApplicationEngine();
    global_engine->rootContext()->setContextProperty("smsBackend", &backend);
    QObject::connect(
        global_engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    global_engine->loadFromModule("smsapp", "Main");

    return QGuiApplication::exec();
}

