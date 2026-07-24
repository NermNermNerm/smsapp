#include "startupmenumanager.h"

static QString autostartDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
    + "/autostart";
}

static QString autostartFileFor(const QString &deviceId)
{
    const QString base = QCoreApplication::applicationName();   // e.g. "sms-app"
    const QString dir  = autostartDir();

    if (deviceId.isEmpty())
        return dir + "/" + base + ".desktop";

    return dir + "/" + base + "-device-" + deviceId + ".desktop";
}

static void writeDesktopFile(const QString &path, const QString &deviceId)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

    const QString exe  = QCoreApplication::applicationFilePath();
    const QString name = QCoreApplication::applicationName();

    QTextStream out(&f);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=" << name << "\n";
    out << "Exec=\"" << exe << "\"";

    if (!deviceId.isEmpty())
        out << " --device=" << deviceId;

    out << "\n";
}

StartupMenuManager::StartupMenuManager(const QString &deviceId,
                                       QObject *parent)
    : QObject(parent)
    , m_deviceId(deviceId)
{
}

bool StartupMenuManager::hasStartupApplicationsEntry()
{
    return QFile::exists(autostartFileFor(m_deviceId));
}

void StartupMenuManager::registerStartupApplicationsEntry()
{
    QDir().mkpath(autostartDir());

    if (m_deviceId.isEmpty()) {
        // Generic mode → remove all per‑device entries
        QDir dir(autostartDir());
        const QStringList entries =
            dir.entryList(QStringList() << "sms-app-device-*.desktop");
        for (const QString &e : entries)
            QFile::remove(dir.filePath(e));
    } else {
        // Per‑device mode → remove generic entry
        QFile::remove(autostartFileFor(""));
    }

    writeDesktopFile(autostartFileFor(m_deviceId), m_deviceId);
}

void StartupMenuManager::removeStartupApplicationsEntry()
{
    QFile::remove(autostartFileFor(m_deviceId));
}
