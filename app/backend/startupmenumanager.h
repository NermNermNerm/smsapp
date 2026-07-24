#pragma once

/**
 * @brief Manages creation and removal of XDG autostart entries.
 *
 * This class enforces a single‑mode autostart policy:
 *
 * - If the user wants per‑device startup, we create one autostart entry
 *   per device, each using --device=<id>.
 *
 * - If the user wants generic startup (no device specified), we create
 *   exactly one autostart entry with no --device argument.
 *
 * These modes are mutually exclusive. Creating a per‑device entry
 * automatically removes the generic entry, and creating a generic entry
 * removes all per‑device entries.
 */
class StartupMenuManager : public QObject
{
    Q_OBJECT

public:
    explicit StartupMenuManager(const QString &deviceId,
                                QObject *parent = nullptr);

public slots:
    bool hasStartupApplicationsEntry();
    void registerStartupApplicationsEntry();
    void removeStartupApplicationsEntry();

private:
    QString m_deviceId;
};
