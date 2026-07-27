#include "TrayIconController.h"
#include "backend/devicestatus.h"
#include <QtSvg/QSvgRenderer>
#include "backend/nameresolver.h"

TrayIconController::TrayIconController(QGuiApplication &app, DeviceStatus &deviceStatus, QObject *parent)
    : QObject(parent), m_deviceStatus(deviceStatus), m_app(app)
{
    QObject::connect(&m_deviceStatus, &DeviceStatus::statusChanged,
                     this, &TrayIconController::onDeviceStatusChanged);
    QObject::connect(&m_deviceStatus, &DeviceStatus::handlerChanged,
                     this, &TrayIconController::onMessagesHandlerChanged);

    connect(qApp, &QGuiApplication::applicationStateChanged,
            this, &TrayIconController::onAppStateChanged);
    m_lastActiveTime = QDateTime::currentDateTime(); // not utc, incoming messages are local time.
    m_numNewMessages = 0;
    m_lastReachableTime = {};
    refreshIcon();
    m_tray.show();
    m_lastStatus = m_deviceStatus.status();
    Q_ASSERT(m_lastStatus != DeviceStatus::Status::DeviceReady);
}

void TrayIconController::onDeviceStatusChanged()
{
    auto newStatus = m_deviceStatus.status();
    Q_ASSERT(newStatus != m_lastStatus);
    if (m_lastStatus == DeviceStatus::Status::DeviceReady) {
        m_lastReachableTime = QDateTime::currentDateTime();
    }
    m_lastStatus = newStatus;
    refreshIcon();
}

void TrayIconController::onAppStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive) {
        // app just became active
        m_numNewMessages = 0;
        m_lastMessageFrom = "";
    } else {
        // app just lost focus
        m_lastActiveTime = QDateTime::currentDateTime();
    }
    refreshIcon();
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
    if (m_app.applicationState() != Qt::ApplicationActive
     && updatedMessage.date() > m_lastActiveTime.toMSecsSinceEpoch()) {
        auto messages = updatedMessage.addresses();
        m_lastMessageFrom = NameResolver::phoneNumberToName(messages.first().address());
        ++m_numNewMessages;
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
            fill="%1" fill-opacity="0.9"/>

      <rect x="22" y="54" width="20" height="4" rx="2" ry="2"
            fill="#202020"/>

      <circle cx="46" cy="48" r="8" fill="%2"/>

      %3
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

static QString formatTime(const QDateTime &dt)
{
    Q_ASSERT(dt.isValid());

    if (dt.secsTo(QDateTime::currentDateTime()) < 24 * 3600)
        return QLocale().toString(dt.time(), QLocale::ShortFormat);
    else
        return QLocale().toString(dt.date(), QLocale::ShortFormat);
}

void TrayIconController::refreshIcon()
{
    static const QColor nonSpecificPhoneColor = QColor("blue");

    QIcon icon = makeTrayIcon(nonSpecificPhoneColor,
                              (m_deviceStatus.deviceName().isEmpty()),
                              (m_deviceStatus.status() == DeviceStatus::Status::DeviceReady),
                              m_numNewMessages > 0);

    QString deviceName = m_deviceStatus.deviceName();

    QString toolTipText;
    if (deviceName.isEmpty()) {
        toolTipText = QStringLiteral("Can't connect to the phone!\nOpen app for details.");
    } else {
        toolTipText = deviceName % "\n";

        //
        // Connection + battery
        //
        if (m_deviceStatus.status() == DeviceStatus::Status::DeviceReady) {
            if (m_deviceStatus.isCharging()) {
                toolTipText += QObject::tr("Connected — %1% and charging\n")
                                   .arg(m_deviceStatus.batteryCharge());
            } else {
                toolTipText += QObject::tr("Connected — %1%\n")
                                   .arg(m_deviceStatus.batteryCharge());
            }
        } else if (m_lastReachableTime.isValid()){
            toolTipText += QObject::tr("Phone unreachable since %1\n").arg(formatTime(m_lastReachableTime));
        }
        else {
            toolTipText += QObject::tr("Phone unreachable since startup\n");
        }

        //
        // New‑message summary
        //
        if (m_app.applicationState() != Qt::ApplicationActive) {
            auto sinceTime = formatTime(m_lastActiveTime);
            if (m_numNewMessages == 0) {
                toolTipText += QObject::tr("No new messages since %1").arg(sinceTime);
            } else if (m_numNewMessages == 1) {
                toolTipText += QObject::tr("1 new message since %1\nLast message from %2")
                .arg(sinceTime, m_lastMessageFrom);
            } else {
                toolTipText += QObject::tr("%1 new messages since %2\nLast message from %3")
                .arg(m_numNewMessages).arg(sinceTime, m_lastMessageFrom);
            }
        }
    }

    m_tray.setToolTip(toolTipText);
    m_tray.setIcon(icon);
}
