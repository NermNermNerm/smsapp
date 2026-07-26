#pragma once

#include <QObject>
#include <QHash>
#include <memory>
#include <qsocketnotifier.h>
#include <vector>
#include "deviceconfig.h"

class FakeDeviceConversationsInterface;
class FakeKdeConnectBattery;

class FakeKdeConnectDaemon : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")

public:
    explicit FakeKdeConnectDaemon(QObject *parent = nullptr);

    bool isValid() const;

    void loadDevices();
    bool registerOnDBus();

    // interactive entrypoint (starts command handling)
    void startInteractive();

    std::vector<std::unique_ptr<DeviceConfig>> &getDeviceConfigs() { return m_devices; };
    FakeDeviceConversationsInterface *getConversationsInterface(const QString &deviceID) { return m_fakeConversations[deviceID]; }
    FakeKdeConnectBattery *getBatteryInterface(const QString &deviceID) { return m_fakeBattery[deviceID]; }

public slots:
    QStringList devices();
    QStringList devices(bool onlyReachable);
    QStringList devices(bool onlyReachable, bool onlyPaired);

    // Cancel ongoing activity on the dbus. (Not part of real interface)
    void reset();

private:
    bool checkSelected() const;

    QString m_serviceName;
    std::vector<std::unique_ptr<DeviceConfig>> m_devices;
    int m_selectedIndex = -1;
    QHash<QString, FakeDeviceConversationsInterface*> m_fakeConversations;
    QHash<QString, FakeKdeConnectBattery*> m_fakeBattery;
};
