#include <QObject>
#include "TrayIconController.h"
#include <QPixmap>
#include <QRandomGenerator>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QDir>

TrayIconController::TrayIconController(SmsBackend &backend, QObject *parent)
    : QObject(parent), m_backEnd(backend)
{
    QObject::connect(&backend, &SmsBackend::deviceStatusChanged,
                     this, &TrayIconController::refreshIcon);
    QObject::connect(&backend, &SmsBackend::unreadMessageCountChanged,
                     this, &TrayIconController::refreshIcon);

    refreshIcon();
    m_tray.show();
}

void TrayIconController::refreshIcon()
{
    QString path = ":/icons/tray_";
    switch (m_backEnd.rawDeviceStatus())
    {
    case SmsBackend::Status::Ok:
        path += "connected";
        break;
    case SmsBackend::Status::DeviceUnreachable:
        path += "disconnected";
        break;
    default:
        path += "error";
        break;
    }

    path += "_";
    path += m_backEnd.unreadMessageCount() > 0 ? "msgs" : "nomsgs";
    path += ".svg";
    QIcon icon(path);
    m_tray.setToolTip("zomg the tooltip works");
    m_tray.setIcon(icon);
}
