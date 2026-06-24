#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QPainter>

class DeviceStatus;

class TrayIconController : public QObject
{
    Q_OBJECT

public:
    explicit TrayIconController(DeviceStatus &deviceStatus, QObject *parent = nullptr);

private:
    void refreshIcon();

    void drawBadge(QPainter &p, const QColor &color, int count);

    DeviceStatus &m_deviceStatus;
    QSystemTrayIcon m_tray;
};
