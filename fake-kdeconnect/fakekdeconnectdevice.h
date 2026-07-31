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

    void setReachable(bool isReachable);

public slots:
    bool hasPlugin(const QString &plugin_name) const;

signals:
    void nameChanged(const QString &name); // not implemented
    void pairStateChanged(int pairState); // not implemented
    void pairingFailed(const QString &error); // not implemented
    void pluginsChanged();
    void reachableChanged(bool reachable);
    void statusIconNameChanged(); // not implemented
    void typeChanged(const QString &type); // not implemented

private:
    DeviceConfig *m_info;
};
