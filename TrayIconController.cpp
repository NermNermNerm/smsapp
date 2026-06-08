#include <QObject>
#include "TrayIconController.h"
#include <QPixmap>
#include <QRandomGenerator>
#include <QIcon>
#include <QMenu>
#include <QAction>

TrayIconController::TrayIconController(SmsBackend &backend, QObject *parent)
    : QObject(parent), m_backEnd(backend)
{
    QIcon icon = QIcon::fromTheme("anydesk");
    auto s = icon.name();
    m_tray.setIcon(icon);

    m_tray.setToolTip("zomg the tooltip works");

    // m_tray.setIcon(QIcon(":/icons/app-base.svg"));
    m_tray.show();

    QObject::connect(&backend, &SmsBackend::deviceStatusChanged,
                     this, &TrayIconController::refreshIcon);
    QObject::connect(&backend, &SmsBackend::unreadMessageCountChanged,
                     this, &TrayIconController::refreshIcon);
}

void TrayIconController::refreshIcon()
{
    // LAND MINE!  This depends on having the breeze icon library installed,
    //   which it won't be for most users or even beta-testers.
    //   TODO: Copy the icon into a resource to remove this dependency.

    // QPixmap pix = QIcon(":/icons/app-base.svg").pixmap(32, 32);
    QPixmap pix = QIcon::fromTheme("anydesk").pixmap(32, 32);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QColor color;
    switch (m_backEnd.rawDeviceStatus())
    {
    case SmsBackend::Status::Ok:
        color = Qt::green;
        break;
    case SmsBackend::Status::DeviceUnreachable:
        color = Qt::yellow;
        break;
    default:
        color = Qt::red;
        break;
    }

    drawBadge(p, color, m_backEnd.unreadMessageCount());

    p.end();

    QString path = QString("/tmp/smsapp-icon-%1.png")
                        .arg(QRandomGenerator::global()->generate());

    pix.save(path, "PNG");

    m_tray.setIcon(QIcon(pix));
}

void TrayIconController::drawBadge(QPainter &p, const QColor &color, int count)
{
    const int size = 32;
    const int radius = 8;

    QRect circle(size - radius*2 - 2, 2, radius*2, radius*2);

    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(circle);

    if (count > 0) {
        p.setPen(Qt::white);
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(9);
        p.setFont(f);

        QString text;
        if (count <= 9)
            text = QString::number(count);
        else
            text = "+";

        p.drawText(circle, Qt::AlignCenter, text);
    }
}
