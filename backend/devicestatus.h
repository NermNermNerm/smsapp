#pragma once
#include "settings.h"
#include <QObject>
#include <QString>
#include <QList>
#include <QPointer>
#include <QTimer>
#include "messageshandler.h"

class MessagesHandler;

class DeviceStatus : public QObject
{
    Q_OBJECT

public:
    struct DeviceInfo {
        QString id;
        QString name;
    };

    enum class Status {
        DaemonNotRunning,
        DaemonHung,
        NoSmsDevice,
        DeviceUnreachable,
        DeviceReady
    };
    Q_ENUM(Status)

    explicit DeviceStatus(QObject *parent = nullptr);

    // List of devices that support SMS
    Q_PROPERTY(QList<DeviceInfo> validDevices READ validDevices NOTIFY validDevicesChanged)

    Q_PROPERTY(QString preferredDevice READ preferredDevice
                   WRITE setPreferredDevice NOTIFY preferredDeviceChanged)
    Q_PROPERTY(QString preferredDeviceName READ preferredDeviceName
                   NOTIFY preferredDeviceNameChanged)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

    Q_PROPERTY(MessagesHandler* handler READ handler NOTIFY handlerChanged)

    Q_PROPERTY(bool autoFixDaemon READ autoFixDaemon
                   WRITE setAutoFixDaemon NOTIFY autoFixDaemonChanged)

public:
    // Accessors
    QList<DeviceInfo> validDevices() const { return m_validDevices; }
    QString preferredDevice() const { return settings().preferredDeviceId(); }
    QString preferredDeviceName() const { return m_preferredDeviceName; }
    Status status() const { return m_status; }
    MessagesHandler* handler() const { return m_handler; }
    bool autoFixDaemon() const {  return settings().autoFixDaemon(); }

    // Mutators
    void setAutoFixDaemon(bool enabled);
    void setPreferredDevice(const QString &id);

    // User-triggered action
    Q_INVOKABLE void rebootDaemon();

signals:
    void validDevicesChanged();
    void preferredDeviceChanged();
    void statusChanged();
    void handlerChanged();
    void autoFixDaemonChanged();
    void preferredDeviceNameChanged();

private:
    void poll();
    void onDeviceListChanged();
    bool tryRefreshDeviceList();
    void trySetupPreferredDevice();

    void setPreferredDeviceName(const QString &name);
    void setStatus(Status status);

    // Ensures that m_handler matches what's in preferredDevice() and changes it if needed.
    void updateHandler();

    Settings &settings() const { return m_settings ? *m_settings : Settings::instance(); }

private:
    QList<DeviceInfo> m_validDevices;
    Status m_status = Status::DaemonNotRunning;
    QPointer<MessagesHandler> m_handler;
    QTimer m_pollTimer;
    Settings *m_settings = nullptr;
    QString m_preferredDeviceName;
    QDateTime m_lastWakeAttempt;


    // Check device status using this as the rough interval - note that if the MessagesHandler has had
    //   activity, that counts as a successful poll.  This is the interval to use when the app is not
    //   minimized and the window is not obscored.
    static constexpr int PollIntervalWhenActiveInMs = 2000;
    // This is the interval to pause between polls when the app is minimized or the window is obscured.
    static constexpr int PollIntervalInBackgroundPoll = 60000;
};
