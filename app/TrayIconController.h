#pragma once

class DeviceStatus;
class ConversationMessage;

class TrayIconController : public QObject
{
    Q_OBJECT

public:
    explicit TrayIconController(QGuiApplication &app, DeviceStatus &deviceStatus, QObject *parent = nullptr);

private:
    void refreshIcon();
    void onMessagesHandlerChanged();
    void onMessageArrived();
    void onAppStateChanged(Qt::ApplicationState state);
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);


    DeviceStatus &m_deviceStatus;
    QSystemTrayIcon m_tray;
    bool m_handlerIsAttached = false;
    QDateTime m_lastActiveTime;
    bool m_hasNewMessages = false;
    QGuiApplication &m_app;
};
