#pragma once

#include "backend/SmsBackend.h"
#include <QObject>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QPainter>

class TrayIconController : public QObject
{
    Q_OBJECT

public:
    explicit TrayIconController(SmsBackend &backend, QObject *parent = nullptr);

private:
    void refreshIcon();

    void drawBadge(QPainter &p, const QColor &color, int count);

    SmsBackend &m_backEnd;
    QSystemTrayIcon m_tray;
};
