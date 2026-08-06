#pragma once
#include "settings.h"
#include "messageshandler.h"
#include "startupmenumanager.h"

class MessagesHandler;
struct DeviceInfo {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString buttonIconUrl MEMBER buttonIconUrl)

public:
    QString id;
    QString name;
    QString buttonIconUrl;
};


class DeviceStatus : public QObject
{
    Q_OBJECT

public:
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

    Q_PROPERTY(QList<DeviceInfo> otherDevices READ otherDevices NOTIFY otherDevicesChanged FINAL)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceNameChanged FINAL)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(MessagesHandler* handler READ handler NOTIFY handlerChanged FINAL)
    Q_PROPERTY(bool autoFixDaemon READ autoFixDaemon WRITE setAutoFixDaemon NOTIFY autoFixDaemonChanged FINAL)
    Q_PROPERTY(int batteryCharge READ batteryCharge NOTIFY batteryChargeChanged FINAL)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY isChargingChanged FINAL)

public:
    // Accessors
    Status status() const { return m_status; }
    MessagesHandler* handler() const { return m_handler; }
    bool autoFixDaemon() const {  return settings().autoFixDaemon(); }
    QList<DeviceInfo> otherDevices() const { return m_otherDevices; }
    QString deviceName() const { return m_deviceName; }
    int batteryCharge() const { return m_batteryCharge; }
    bool isCharging() const { return m_isCharging; }
    QString specifiedDeviceId() const { return m_specifiedDeviceId; }
    bool isMultiDeviceMode() const { return false; /* TOOD: */ }

    // Mutators
    void setAutoFixDaemon(bool enabled);

    static DeviceStatus *instance() {
        Q_ASSERT(DeviceStatus::s_instance != nullptr);
        return DeviceStatus::s_instance;
    }

public slots:
    void rebootDaemon();
    void launchOtherDevice(const QString &id);

signals:
    void statusChanged();
    void handlerChanged();
    void autoFixDaemonChanged();
    void deviceNameChanged();
    void otherDevicesChanged();
    void batteryChargeChanged();
    void isChargingChanged();

private:
    void poll();
    void onDeviceListChanged();
    bool tryRefreshDeviceList();
    void setDeviceName(const QString &name);
    void setStatus(Status status);
    void setupHandler(const QString &deviceId);
    QString makeButtonImageUrl(const QString &id) const;
    void handleDevice();
    Settings &settings() const { return m_settings ? *m_settings : Settings::instance(); }
    StartupMenuManager &startupManager() const { return *StartupMenuManager::instance(); }

    QList<DeviceInfo> m_otherDevices;
    Status m_status = Status::DaemonNotRunning;
    QPointer<MessagesHandler> m_handler;
    QTimer m_pollTimer;
    Settings *m_settings = nullptr;
    QDateTime m_lastWakeAttempt;
    QString m_deviceName;
    int m_batteryCharge; // 0-100
    bool m_isCharging;

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
