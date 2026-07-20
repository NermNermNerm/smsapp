#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QtLogging>
#include "fakekdeconnectdaemon.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("fake-kdeconnectd");

    FakeKdeConnectDaemon daemon;
    daemon.loadDevices();
    if (!daemon.registerOnDBus())
    {
        qCritical() << "Couldn't register the fake device - maybe another copy of this app is already running?";
        return 1;
    }

    if (!daemon.isValid()) {
        qCritical() << "Daemon state is not valid.";
        return 1;
    }

    // interactive mode
    QTimer::singleShot(0, &daemon, &FakeKdeConnectDaemon::startInteractive);
    return app.exec();
}
