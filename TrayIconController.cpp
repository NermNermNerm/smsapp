#include <QObject>
#include "TrayIconController.h"
#include "backend/devicestatus.h"
#include <QPixmap>
#include <QRandomGenerator>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QDir>

TrayIconController::TrayIconController(DeviceStatus &deviceStatus, QObject *parent)
    : QObject(parent), m_deviceStatus(deviceStatus)
{
    QObject::connect(&m_deviceStatus, &DeviceStatus::statusChanged,
                     this, &TrayIconController::refreshIcon);
    QObject::connect(&m_deviceStatus, &DeviceStatus::handlerChanged,
                     this, &TrayIconController::refreshIcon);

    refreshIcon();
    m_tray.show();
}

void TrayIconController::refreshIcon()
{
    QString path = ":/icons/tray_";
    switch (m_deviceStatus.status())
    {
    case DeviceStatus::Status::DeviceReady:
        path += "connected";
        break;
    case DeviceStatus::Status::DeviceUnreachable:
        path += "disconnected";
        break;
    default:
        path += "error";
        break;
    }

    // TODO: Reconsider how to do this.
    int unreadCount = 0; // m_deviceStatus.handler() ? m_deviceStatus.handler()->unreadMessageCount() : 0;
    path += "_";
    path += unreadCount > 0 ? "msgs" : "nomsgs";
    path += ".svg";
    QIcon icon(path);
    m_tray.setToolTip("zomg the tooltip works");
    m_tray.setIcon(icon);
}
