#pragma once

class DeviceStatus;
class Main;

/**
 * @brief Manages creation and removal of XDG autostart entries.
 */
class StartupMenuManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isPinned READ isPinned WRITE setIsPinned NOTIFY isPinnedChanged FINAL)

public:
    static StartupMenuManager &instance();

    void removeAutoStart(const QString &deviceId = "");
    bool isPinned() const;
    void setIsPinned(bool isPinned);

signals:
    void isPinnedChanged();

private:
    explicit StartupMenuManager(QObject *parent = nullptr);
    void setMultiMode();
    void ensureFileStateMatchesInMemoryState() const;

    mutable bool m_isPinned = false;
    mutable bool m_isPinnedIsValid = false;
    mutable bool m_isMultiModeValid = false;
    mutable bool m_isMultiMode = false;

    DeviceStatus &deviceStatus() const;
    Main &main() const;
};
