#include "TrayIconController.h"
#include "backend/devicestatus.h"
#include <QtSvg/QSvgRenderer>
#include <QApplication>

TrayIconController::TrayIconController(QGuiApplication &app, DeviceStatus &deviceStatus, QObject *parent)
    : QObject(parent), m_deviceStatus(deviceStatus), m_app(app)
{
    QObject::connect(&m_deviceStatus, &DeviceStatus::statusChanged,
                     this, &TrayIconController::refreshIcon);
    QObject::connect(&m_deviceStatus, &DeviceStatus::handlerChanged,
                     this, &TrayIconController::onMessagesHandlerChanged);

    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &TrayIconController::onAppStateChanged);
    m_lastActiveTime = QDateTime::currentDateTime(); // not utc, incoming messages are local time.
    m_hasNewMessages = false;
    refreshIcon();
    m_tray.show();
}

void TrayIconController::onAppStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive) {
        // app just became active
        m_hasNewMessages = false;
        refreshIcon();
    } else {
        // app just lost focus
        m_lastActiveTime = QDateTime::currentDateTime();
    }
}

void TrayIconController::onMessagesHandlerChanged()
{
    Q_ASSERT(!m_handlerIsAttached);
    m_handlerIsAttached = true;

    connect(m_deviceStatus.handler(), &MessagesHandler::conversationMessageChanged,
                     this, &TrayIconController::onConversationMessageChanged);
}

void TrayIconController::onConversationMessageChanged(const ConversationMessage &updatedMessage)
{
    if (!m_hasNewMessages
     && m_app.applicationState() != Qt::ApplicationActive
     && updatedMessage.date() > m_lastActiveTime.toMSecsSinceEpoch()) {
        m_hasNewMessages = true;
        refreshIcon();
    }
}


static QIcon makeTrayIcon(const QColor &background,
                          bool isError,
                          bool isReachable,
                          bool hasMessages)
{
    auto renderSvgToPixmap = [&](int size, const QString &svg) {
        QSvgRenderer renderer(svg.toUtf8());
        QImage img(QSize(size, size), QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        renderer.render(&p);
        return QPixmap::fromImage(img);
    };

    // --- Build dynamic SVG ---
    QString screenFill = background.name();

    // Connection dot color
    QString dotColor;
    if (isError)
        dotColor = "#ff0000";      // red
    else if (!isReachable)
        dotColor = "#808080";      // grey
    else
        dotColor = "#00cc44";      // green

    // Message sparkle (optional)
    QString sparkle;
    if (hasMessages) {
        sparkle = R"SVG(
    <g transform="translate(22,16) scale(2.5)">
      <polygon points="0,-5 1,-1 5,0 1,1 0,5 -1,1 -5,0 -1,-1"
               fill="#3498db"/>
    </g>
)SVG";
    }

    // Final SVG template
    QString svg = QString(R"(
    <svg xmlns="http://www.w3.org/2000/svg"
         width="64" height="64" viewBox="0 0 64 64">

      <rect x="10" y="4" width="44" height="56" rx="5" ry="5"
            fill="#ffffff" fill-opacity="0.85"/>

      <rect x="14" y="10" width="36" height="42" rx="3" ry="3"
            fill="%2" fill-opacity="0.9"/>

      <rect x="22" y="54" width="20" height="4" rx="2" ry="2"
            fill="#202020"/>

      <circle cx="46" cy="48" r="8" fill="%3"/>

      %4
    </svg>
    )")
    .arg(screenFill, dotColor, sparkle);

    // --- Build QIcon with multiple sizes ---
    QIcon icon;
    icon.addPixmap(renderSvgToPixmap(16, svg));
    icon.addPixmap(renderSvgToPixmap(22, svg));
    icon.addPixmap(renderSvgToPixmap(32, svg));

    return icon;
}

void TrayIconController::refreshIcon()
{
    static const QColor nonSpecificPhoneColor = QColor("blue");

    QIcon icon = makeTrayIcon(nonSpecificPhoneColor, m_deviceStatus.deviceName() == "", m_deviceStatus.status() == DeviceStatus::Status::DeviceReady, m_hasNewMessages);

    // text builder:
/*
Pixel 7
Connected, 78% (Charging)
3 new messages since 2:14 PM
Last message from John Doe

iPhone 12
Phone unreachable since 2:14pm
2 new messages since 10:13am

Can't connect to phone!
Open app for details.
 */

    m_tray.setToolTip("zomg the tooltip works");
    m_tray.setIcon(icon);
}
