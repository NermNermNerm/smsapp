#pragma once
#include <QObject>

class DeviceConfig;

class FakeKdeConnectDevice : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device")
    Q_PROPERTY(bool isReachable READ isReachable)
    Q_PROPERTY(QString name READ name)

public:
    explicit FakeKdeConnectDevice(DeviceConfig *info, QObject *parent = nullptr);

    bool isReachable() const;
    QString name() const;

public slots:
    bool hasPlugin(const QString &plugin_name) const;

private:
    DeviceConfig *m_info;
};
