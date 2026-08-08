#pragma once

#include "backend/devicestatus.h"

class ConversationMessage;
class OtpScanner;

class TrayIconController : public QObject
{
    Q_OBJECT

public:
    static TrayIconController &instance();

    Q_INVOKABLE QScreen *getScreen() const;

private:
    explicit TrayIconController(QObject *parent = nullptr);
    void refreshIcon();
    void onDeviceStatusChanged();
    void onMessagesHandlerChanged();
    void onMessageArrived();
    void onAppStateChanged(Qt::ApplicationState state);
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    DeviceStatus &deviceStatus() const;

    QSystemTrayIcon m_tray;
    bool m_handlerIsAttached = false;
    QDateTime m_lastActiveTime;
    QDateTime m_lastReachableTime = {};
    int m_numNewMessages = 0;
    QString m_lastMessageFrom;
    DeviceStatus::Status m_lastStatus;
    mutable QScreen *m_lastTrayScreen;
};
