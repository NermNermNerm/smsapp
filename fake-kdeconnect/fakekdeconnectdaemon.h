#pragma once

#include <QObject>
#include <QHash>
#include <memory>
#include <qsocketnotifier.h>
#include <vector>
#include "deviceconfig.h"

class FakeDeviceConversationsInterface;
class FakeKdeConnectBattery;
class FakeKdeConnectDevice;

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
    FakeKdeConnectDevice *getDeviceInterface(const QString &deviceID) { return m_fakeDevices[deviceID]; }
    FakeKdeConnectBattery *getBatteryInterface(const QString &deviceID) { return m_fakeBattery[deviceID]; }

    /**  @brief Creates a new device and returns the index of the new device */
    int newDevice(const DeviceConfig &config);
    void removeDevice(int deviceIndex);
    int restoreDevice(const QString &id);

public slots:
    QStringList devices();
    QStringList devices(bool onlyReachable);
    QStringList devices(bool onlyReachable, bool onlyPaired);

    // Cancel ongoing activity on the dbus. (Not part of real interface)
    void reset();

signals:
    void announcedNameChanged(const QString &announcedName); // not implemented
    void customDevicesChanged(const QStringList &customDevices); // not implemented
    void deviceAdded(const QString &id);
    void deviceListChanged();
    void deviceRemoved(const QString &id); // not implemented (yet)
    void deviceVisibilityChanged(const QString &id, bool isVisible); // not implemented
    void pairingRequestsChanged(); // not implemented

private:
    bool checkSelected() const;
    bool registerDevice(DeviceConfig *device);

    QString m_serviceName;
    std::vector<std::unique_ptr<DeviceConfig>> m_devices;
    int m_selectedIndex = -1;
    QHash<QString, FakeKdeConnectDevice*> m_fakeDevices;
    QHash<QString, FakeDeviceConversationsInterface*> m_fakeConversations;
    QHash<QString, FakeKdeConnectBattery*> m_fakeBattery;
};
