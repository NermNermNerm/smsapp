#pragma once

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings& instance();

    QString preferredDeviceId() const;
    void setPreferredDeviceId(const QString &id);

    bool autoFixDaemon() const;
    void setAutoFixDaemon(bool enabled);

private:
    Settings(bool isUsingFakeDBus);
    QSettings m_settings;
};
