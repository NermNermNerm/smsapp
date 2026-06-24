#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QPainter>
#include <QIcon>
#include "TrayIconController.h"
#include "backend/nameresolver.h"
#include "backend/devicestatus.h"
#include "backend/conversationlistmodel.h"

static QQmlApplicationEngine *global_engine = nullptr;
static QQuickWindow *my_qml_window = nullptr;

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

    NameResolver::load();
    qmlRegisterType<DeviceStatus>("Sms", 1, 0, "DeviceStatus");
    qmlRegisterType<ConversationListModel>("Sms", 1, 0, "ConversationListModel");

    DeviceStatus deviceStatus;
    ConversationListModel conversations;

    QObject::connect(&deviceStatus, &DeviceStatus::handlerChanged,
                     &conversations, [&]() {
        conversations.setDevice(deviceStatus.handler());
    });

    TrayIconController tray(deviceStatus);

    QQmlApplicationEngine global_engine;
    global_engine.rootContext()->setContextProperty("deviceStatus", &deviceStatus);
    global_engine.rootContext()->setContextProperty("conversations", &conversations);
    QObject::connect(
        &global_engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    global_engine.loadFromModule("smsapp", "Main");

    return QGuiApplication::exec();
}

