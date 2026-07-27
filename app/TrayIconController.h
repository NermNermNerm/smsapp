#pragma once
#include "backend/devicestatus.h"

class DeviceStatus;
class ConversationMessage;

class TrayIconController : public QObject
{
    Q_OBJECT

public:
    explicit TrayIconController(QGuiApplication &app, DeviceStatus &deviceStatus, QObject *parent = nullptr);

private:
    void refreshIcon();
    void onDeviceStatusChanged();
    void onMessagesHandlerChanged();
    void onMessageArrived();
    void onAppStateChanged(Qt::ApplicationState state);
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);


    DeviceStatus &m_deviceStatus;
    QSystemTrayIcon m_tray;
    bool m_handlerIsAttached = false;
    QDateTime m_lastActiveTime;
    QDateTime m_lastReachableTime = {};
    int m_numNewMessages = 0;
    QString m_lastMessageFrom;
    QGuiApplication &m_app;
    DeviceStatus::Status m_lastStatus;
};
