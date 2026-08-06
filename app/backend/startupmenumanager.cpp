#include "startupmenumanager.h"
#include "backend/devicestatus.h"

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

StartupMenuManager::StartupMenuManager(QObject *parent)
    : QObject(parent)
{
    // We hook onto both handlerChanged and otherDevicesChanged because If there are no devices
    //  when we get hooked up, we won't get notified (because it's going from 0 (the default) to 0.
    connect(&deviceStatus(), &DeviceStatus::handlerChanged, this, &StartupMenuManager::setMultiMode);
    connect(&deviceStatus(), &DeviceStatus::otherDevicesChanged, this, &StartupMenuManager::setMultiMode);
}

bool StartupMenuManager::isPinned() const
{
    if (m_isPinnedIsValid) {
        return m_isPinned;
    }

    if (deviceStatus().handler() == nullptr) {
        return false;
    }

    if (!m_isMultiModeValid) {
        m_isMultiMode = !deviceStatus().otherDevices().empty();
        m_isMultiModeValid = true;
        qInfo() << "StartupMenuManager::isPinned set m_isMultiMode to " << m_isMultiMode;
    }

    m_isPinned = QFile::exists(autostartFileFor(deviceStatus().handler()->deviceID()))
                 || (deviceStatus().specifiedDeviceId().isEmpty() && QFile::exists(autostartFileFor("")));
    m_isPinnedIsValid = true;
    qInfo() << "StartupMenuManager::isPinned set m_isPinned to " << m_isPinned;
    return m_isPinned;
}

void StartupMenuManager::setIsPinned(bool isPinned)
{
    if (m_isPinnedIsValid && m_isPinned == isPinned)
        return;

    Q_ASSERT(deviceStatus().handler() != nullptr); // UI Should hide the button if we're not initialized.
    if (deviceStatus().handler() == nullptr)
        return;

    m_isPinnedIsValid = true;
    m_isPinned = isPinned;
    qInfo() << "StartupMenuManager::setIsPinned set m_isPinned to " << m_isPinned;
    ensureFileStateMatchesInMemoryState();
    emit isPinnedChanged();
}

void StartupMenuManager::setMultiMode()
{
    bool isMultiMode = !deviceStatus().otherDevices().isEmpty();
    // !m_isPinnedIsValid guards against the case where otherDevicesChanged is raised before
    // the handler is attached.  It ensures we call ensure... on startup.  We want to be
    // sure and do that because the devices list could have changed when we weren't running.
    if (!m_isMultiModeValid || m_isMultiMode != isMultiMode || !m_isPinnedIsValid) {
        m_isMultiMode = isMultiMode;
        m_isMultiModeValid = true;
        qInfo() << "StartupMenuManager::setMultiMode set multiMode to " << m_isMultiMode;

        ensureFileStateMatchesInMemoryState();
        emit isPinnedChanged();
    }
}

void StartupMenuManager::ensureFileStateMatchesInMemoryState() const
{
    isPinned();

    if (!m_isPinnedIsValid || !m_isMultiModeValid || deviceStatus().handler() == nullptr) {
        return; // Too early in the cycle yet...
    }

    if ((!m_isPinned && deviceStatus().specifiedDeviceId().isEmpty()) || m_isMultiMode) {
        QFile::remove(autostartFileFor(""));
        qInfo() << "StartupMenuManager::ensureFileStateMatchesInMemoryState removed " << autostartFileFor("");
    }

    QString deviceId = deviceStatus().handler()->deviceID();
    if (!m_isPinned || !m_isMultiMode) {
        QFile::remove(autostartFileFor(deviceId));
        qInfo() << "StartupMenuManager::ensureFileStateMatchesInMemoryState removed " << autostartFileFor(deviceId);
    }

    if (m_isPinned) {
        writeDesktopFile(autostartFileFor(m_isMultiMode ? deviceId : ""), deviceId);
        qInfo() << "StartupMenuManager::ensureFileStateMatchesInMemoryState wrote " << autostartFileFor(m_isMultiMode ? deviceId : "");
    }
}

void StartupMenuManager::removeAutoStart(const QString &deviceId)
{
    QFile::remove(autostartFileFor(deviceId));
    qInfo() << "StartupMenuManager::removeAutoStart removed" << autostartFileFor(deviceId);
}

DeviceStatus &StartupMenuManager::deviceStatus() const
{
    return *DeviceStatus::instance();
}

StartupMenuManager *StartupMenuManager::s_instance = nullptr;
StartupMenuManager *StartupMenuManager::instance()
{
    if (s_instance == nullptr)
        s_instance = new StartupMenuManager();
    return s_instance;
}
