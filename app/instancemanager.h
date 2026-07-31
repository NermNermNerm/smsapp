#pragma once

class InstanceManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief If no other instance is managing the given deviceId, register
     *        a DBus service for it and return.
     *
     *        If another instance is already managing it, ask it to raise its
     *        window and exit the app.
     */
    static void claimOrExit(const QString &deviceId);

public slots:
    void RaiseWindow() {
        const auto windows = QGuiApplication::allWindows();
        if (!windows.isEmpty())
            windows.first()->requestActivate();
    }
};