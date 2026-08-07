#include "main.h"
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
#include "installer.h"

Main &Main::instance()
{
    static Main *inst = new Main();
    return *inst;
}

int Main::run(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption deviceOpt(
        "device",
        "The KDEConnect device ID of the phone this instance should talk to.",
        "id"
        );
    parser.addOption(deviceOpt);
    QCommandLineOption startMinimizedOpt("startMinimized", "Start the application minimized");
    parser.addOption(startMinimizedOpt);

    parser.process(app);
    m_specifiedDevice = parser.value(deviceOpt);

    // I'm really not sure what any of this is doing and it's probably not right.
    app.setApplicationName("appsmsapp");
    app.setDesktopFileName("smsapp");
    QGuiApplication::setDesktopFileName("smsapp");
    QCoreApplication::setOrganizationName("NermNermNerm");
    QCoreApplication::setApplicationName("SmsApp");

    NameResolver::load();
    qmlRegisterType<DeviceStatus>("Sms", 1, 0, "DeviceStatus");
    qmlRegisterType<ConversationListModel>("Sms", 1, 0, "ConversationListModel");
    qmlRegisterType<MessageListModel>("Sms", 1, 0, "MessageListModel");
    qmlRegisterType<AvatarModel>("Sms", 1, 0, "AvatarModel");
    qmlRegisterType<SingleAvatarModel>("Sms", 1, 0, "SingleAvatarModel");
    qmlRegisterType<AttachmentListModel>("Sms", 1, 0, "AttachmentListModel");
    qmlRegisterType<OutgoingAttachmentListModel>("Sms", 1, 0, "OutgoingAttachmentListModel");
    qmlRegisterType<StartupMenuManager>("Sms", 1, 0, "StartupMenuManager");

    QQmlApplicationEngine global_engine;
    global_engine.rootContext()->setContextProperty("deviceStatus", &DeviceStatus::instance());
    global_engine.rootContext()->setContextProperty("startupMenuManager", &StartupMenuManager::instance());
    global_engine.rootContext()->setContextProperty("main", &Main::instance());
    global_engine.rootContext()->setContextProperty("conversationListModel", &ConversationListModel::instance());
    global_engine.rootContext()->setContextProperty("messageListModel", &MessageListModel::instance());
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

int main(int argc, char *argv[])
{
    Installer::ensureInstalled(argv);

    // Do this first because we want it done before accessing any settings -- which settings we load
    // depends on whether we're using the fake or real back-end.
    dbus::init();

    return Main::instance().run(argc, argv);
}
