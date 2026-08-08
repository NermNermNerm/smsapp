import QtQuick
import QtQuick.Window
import "."

Window {
    id: toastRoot

    // Default settings
    property alias text: label.text
    property int duration: 3000 // milliseconds before auto-dismiss
    property int margin: 20      // distance from screen edge

    width: 280
    height: 70
    color: "transparent"

    AppTheme {
        id: theme
    }

    // Window flags to remove borders, keep on top, and hide from taskbar
    flags: Qt.ToolTip | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Connections {
        target: otpScanner
        function onOtpReceived(code, actualSender, body, parsedSender) {
            toastRoot.show(qsTr("2FA code %1 from %2 copied to clipboard.")
                           .arg(code)
                           .arg(parsedSender === "" ? actualSender : parsedSender));
        }
    }

    // Main background container
    Rectangle {
        id: body
        anchors.fill: parent
        color: theme.windowBackground
        radius: 8
        border.color: theme.windowBorderColor
        border.width: 1
        opacity: 0

        Row {
            anchors.centerIn: parent
            anchors.margins: 12
            spacing: 12

            Text {
                id: label
                color: theme.text
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                width: toastRoot.width - 40
            }
        }
    }

    // Timer to handle auto-hide
    Timer {
        id: hideTimer
        interval: toastRoot.duration
        repeat: false
        onTriggered: fadeOut.start()
    }

    // Show animation (Fade & Slide up)
    ParallelAnimation {
        id: fadeIn
        NumberAnimation { target: body; property: "opacity"; to: 1.0; duration: 250; easing.type: Easing.OutCubic }
        NumberAnimation { target: toastRoot; property: "y"; to: toastRoot.targetY; duration: 250; easing.type: Easing.OutCubic }
        onFinished: hideTimer.start()
    }

    // Hide animation
    ParallelAnimation {
        id: fadeOut
        NumberAnimation { target: body; property: "opacity"; to: 0.0; duration: 250; easing.type: Easing.InCubic }
        onFinished: toastRoot.visible = false
    }

    property real targetY: 0

    // Call this method to show the toast near the bottom-right corner
    function show(message) {
        if (message) {
            toastRoot.text = message;
        }

        var screen = trayIconController.getScreen() ?? main.getScreen();
        var g = screen.availableGeometry;

        toastRoot.x = g.right - toastRoot.width - toastRoot.margin;

        targetY = g.bottom - toastRoot.height - toastRoot.margin;
        toastRoot.y = targetY + 15;

        toastRoot.visible = true;
        fadeIn.start();
    }
}