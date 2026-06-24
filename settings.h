#pragma once
#include <QObject>
#include <QSettings>

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
    Settings();
    QSettings m_settings;
};
