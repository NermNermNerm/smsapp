#include "TrayIconController.h"
#include "backend/nameresolver.h"
#include "backend/devicestatus.h"
#include "backend/conversationlistmodel.h"
#include "dbus.h"
#include "backend/messagelistmodel.h"
#include "backend/avatarmodel.h"
#include "backend/singleavatarmodel.h"
#include "backend/attachmentlistmodel.h"
#include "backend/outgoingattachmentListmodel.h"
#include "backend/draftmessages.h"
#include "backend/startupmenumanager.h"

static QQmlApplicationEngine *global_engine = nullptr;
static QQuickWindow *my_qml_window = nullptr;

int main(int argc, char *argv[])
{
    // Do this first because we want it done before accessing any settings -- which settings we load
    // depends on whether we're using the fake or real back-end.
    dbus::init();

    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption deviceOpt(
        "device",
        "The KDEConnect device ID of the phone this instance should talk to.",
        "id"
        );
    QCommandLineOption startMinimizedOpt("startMinimized", "Start the application minimized");
    parser.addOption(deviceOpt);

    parser.process(app);
    const QString deviceId = parser.value(deviceOpt);

    StartupMenuManager startupMenuManager(deviceId);
    DeviceStatus deviceStatus(deviceId);

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
    // We've got settings in main.qml and in settings.cpp.
    QGuiApplication::setDesktopFileName("smsapp");
    QCoreApplication::setOrganizationName("NermNermNerm");
    // QCoreApplication::setOrganizationDomain("yourorg.example");
    QCoreApplication::setApplicationName("SmsApp");

    NameResolver::load();
    qmlRegisterType<DeviceStatus>("Sms", 1, 0, "DeviceStatus");
    qmlRegisterType<ConversationListModel>("Sms", 1, 0, "ConversationListModel");
    qmlRegisterType<MessageListModel>("Sms", 1, 0, "MessageListModel");
    qmlRegisterType<AvatarModel>("Sms", 1, 0, "AvatarModel");
    qmlRegisterType<SingleAvatarModel>("Sms", 1, 0, "SingleAvatarModel");
    qmlRegisterType<AttachmentListModel>("Sms", 1, 0, "AttachmentListModel");
    qmlRegisterType<OutgoingAttachmentListModel>("Sms", 1, 0, "OutgoingAttachmentListModel");

    DraftMessages drafts(deviceStatus);
    ConversationListModel conversationListModel(drafts);
    MessageListModel messageListModel(drafts);

    QObject::connect(&deviceStatus, &DeviceStatus::handlerChanged,
                     &conversationListModel, [&]() {
        conversationListModel.setDevice(deviceStatus.handler());
        messageListModel.setDevice(deviceStatus.handler());
    });

    TrayIconController tray(deviceStatus);

    QQmlApplicationEngine global_engine;
    global_engine.rootContext()->setContextProperty("deviceStatus", &deviceStatus);
    global_engine.rootContext()->setContextProperty("conversationListModel", &conversationListModel);
    global_engine.rootContext()->setContextProperty("messageListModel", &messageListModel);
    global_engine.rootContext()->setContextProperty("cliStartMinimized", parser.isSet(startMinimizedOpt));
    QObject::connect(
        &global_engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    global_engine.loadFromModule("smsapp", "Main");

    return QGuiApplication::exec();
}

