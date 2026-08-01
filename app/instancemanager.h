#pragma once

class InstanceManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief If no other instance is managing the given deviceId, register
     *        a DBus service for it and return true.
     *
     *        If another instance is already managing it, ask it to raise its
     *        window and exit the app and return false.  (Exiting does not
     *        happen immediately.)
     */
    static bool claimOrExit(const QString &deviceId);

public slots:
    void activateApp();
};