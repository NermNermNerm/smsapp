#pragma once
#include "settings.h"
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
        /** @brief Indicates that the KDE Service is not properly installed. (not listening on the dbus anyway) */
        DaemonNotRunning,

        /** @brief Dbus messages sent to the kde daemon are not being responded to, but it is running */
        DaemonHung,

        /** @brief The kde service isn't showing any devices that we've talked to before and none that support sms */
        NoSmsDevice,

        /** @brief The specific device we were told to communicate with is non paired in KDE */
        DeviceMissing,

        /** @brief The device we're supposed to talk to is unreachable. */
        DeviceUnreachable,

        /** @brief Good to go. */
        DeviceReady
    };
    Q_ENUM(Status)

    explicit DeviceStatus(const QString &specifiedDeviceId = "", QObject *parent = nullptr);

    // List of devices that support SMS
    Q_PROPERTY(QList<DeviceInfo> otherDevices READ otherDevices NOTIFY otherDevicesChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(MessagesHandler* handler READ handler NOTIFY handlerChanged)
    Q_PROPERTY(bool autoFixDaemon READ autoFixDaemon WRITE setAutoFixDaemon NOTIFY autoFixDaemonChanged)

public:
    // Accessors
    Status status() const { return m_status; }
    MessagesHandler* handler() const { return m_handler; }
    bool autoFixDaemon() const {  return settings().autoFixDaemon(); }
    QList<DeviceInfo> otherDevices() const { return m_otherDevices; }
    QString deviceName() const { return m_deviceName; }

    // Mutators
    void setAutoFixDaemon(bool enabled);
    void setPreferredDevice(const QString &id);

    // User-triggered action
    Q_INVOKABLE void rebootDaemon();

    static DeviceStatus *instance() {
        Q_ASSERT(DeviceStatus::s_instance != nullptr);
        return DeviceStatus::s_instance;
    }

signals:
    void statusChanged();
    void handlerChanged();
    void autoFixDaemonChanged();
    void deviceNameChanged();
    void otherDevicesChanged();

private:
    void poll();
    void onDeviceListChanged();
    bool tryRefreshDeviceList();
    void setDeviceName(const QString &name);

    void setStatus(Status status);

    void setupHandler(const QString &deviceId);

    Settings &settings() const { return m_settings ? *m_settings : Settings::instance(); }

private:
    QList<DeviceInfo> m_otherDevices;
    Status m_status = Status::DaemonNotRunning;
    QPointer<MessagesHandler> m_handler;
    QTimer m_pollTimer;
    Settings *m_settings = nullptr;
    QDateTime m_lastWakeAttempt;
    QString m_deviceName;

    /** @brief if we were told on the command line what device to interact with, this is it.  Else it's empty */
    const QString m_specifiedDeviceId;

    static DeviceStatus *s_instance;


    // Check device status using this as the rough interval - note that if the MessagesHandler has had
    //   activity, that counts as a successful poll.  This is the interval to use when the app is not
    //   minimized and the window is not obscored.
    static constexpr int PollIntervalWhenActiveInMs = 2000;
    // This is the interval to pause between polls when the app is minimized or the window is obscured.
    static constexpr int PollIntervalInBackgroundPoll = 60000;
};
