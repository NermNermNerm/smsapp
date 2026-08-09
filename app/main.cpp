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
#include "backend/soundcontroller.h"
#include "backend/otpscanner.h"
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
    m_startMinimized = parser.isSet(startMinimizedOpt);

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
    global_engine.rootContext()->setContextProperty("otpScanner", &OtpScanner::instance());

    // This isn't actually useful to QML, but who knows, maybe someday, and we need to force it to create an instance.
    global_engine.rootContext()->setContextProperty("trayIconController", &TrayIconController::instance());
    global_engine.rootContext()->setContextProperty("soundController", &SoundController::instance());

    QObject::connect(
        &global_engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    global_engine.loadFromModule("smsapp", "Main");

    return QGuiApplication::exec();
}

QWindow *Main::getWindow() const
{
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *w : windows) {
        auto n = w->objectName();
        if (w->objectName() == "mainWindow")
            return w;
    }
    Q_ASSERT(false); // probably means somebody deleted or changed the 'id: ' line in main.qml if it fails.
    return nullptr;
}

QScreen *Main::getScreen() const
{
    return getWindow()->screen();
}

int main(int argc, char *argv[])
{
    Installer::ensureInstalled(argv);

    // Do this first because we want it done before accessing any settings -- which settings we load
    // depends on whether we're using the fake or real back-end.
    dbus::init();

    return Main::instance().run(argc, argv);
}
