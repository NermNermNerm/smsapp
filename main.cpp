#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "backend/SmsBackend.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<SmsBackend>("Sms", 1, 0, "SmsBackend");
    SmsBackend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("smsBackend", &backend);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("smsapp", "Main");

    return QGuiApplication::exec();
}
