#pragma once

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings& instance();

    QString previousSessionDeviceId() const;
    void setPreviousSessionDeviceId(const QString &id);

    bool autoFixDaemon() const;
    void setAutoFixDaemon(bool enabled);
    bool isDeviceKnownToHaveSms(const QString &deviceID) const;
    void setDeviceKnownToHaveSms(const QString &deviceID);

private:
    Settings(bool isUsingFakeDBus);
    QSettings m_settings;
};
