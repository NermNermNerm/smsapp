import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Sms 1.0
import "."

ApplicationWindow {
    id: mainWindow
    width: 940
    height: 480
    visible: true
    flags: Qt.FramelessWindowHint | Qt.Window

    title: qsTr("Texts – %1").arg(deviceStatus.deviceName)

    Settings {
        id: windowSettings
        category: "Window-" + specifiedDeviceId
        property alias x: mainWindow.x
        property alias y: mainWindow.y
        property alias width: mainWindow.width
        property alias height: mainWindow.height
        property int conversationListWidth: 200
    }

    Component.onCompleted: {
        if (mainWindow.x < screen.virtualX || mainWindow.x > (screen.virtualX + screen.width - 100)) {
            mainWindow.x = screen.virtualX + (screen.width - mainWindow.width) / 2
        }
        if (mainWindow.y < screen.virtualY || mainWindow.y > (screen.virtualY + screen.height - 100)) {
            mainWindow.y = screen.virtualY + (screen.height - mainWindow.height) / 2
        }
    }

    AppTheme {
        id: theme
    }

    // -------------------------------------------------------------------------
    // Custom Title Bar
    // -------------------------------------------------------------------------
    Rectangle {
        id: titleBar
        height: 38
        width: parent.width
        color: theme.titleBarBackground
        anchors.top: parent.top

        // Bottom subtle border for separation from content
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: theme.titleBarBorder
        }

        // Background Drag & Double-Click Handler
        MouseArea {
            id: moveWindowMouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton

            onPressed: {
                // Uses native OS window movement for smooth drag & snapping
                mainWindow.startSystemMove()
            }

            onDoubleClicked: {
                if (mainWindow.visibility === Window.Maximized)
                    mainWindow.showNormal()
                else
                    mainWindow.showMaximized()
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 0
            spacing: 8

            // --- Status Dot ---
            Rectangle {
                visible: deviceStatus.status !== DeviceStatus.DeviceReady
                width: 10
                height: 10
                radius: 5
                Layout.alignment: Qt.AlignVCenter
                color: deviceStatus.status === DeviceStatus.DeviceUnreachable ? theme.statusWarn : theme.statusError
            }

            BatteryIndicator {
                visible: deviceStatus.status === DeviceStatus.DeviceReady
                showText: false
                height: 16
                isCharging: deviceStatus.isCharging
                charge: deviceStatus.batteryCharge
            }

            // --- Active Phone Name ---
            Label {
                text: deviceStatus.deviceName !== "" ? deviceStatus.deviceName : qsTr("No SMS Device Available")
                font.bold: true
                font.pixelSize: 13
                color: theme.titleBarButtonTextColor
                Layout.alignment: Qt.AlignVCenter
                Layout.rightMargin: 6
            }


            // Flexible Spacer (pushes other devices to the middle)
            Item {
                Layout.fillWidth: true
            }

            Repeater {
                model: deviceStatus.otherDevices

                delegate: ToolButton {
                    implicitWidth: 40
                    implicitHeight: 40

                    contentItem: Image {
                        anchors.centerIn: parent
                        source: modelData.buttonIconUrl
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: `Starts the SMS App for '${modelData.name}'`

                    onClicked: deviceStatus.launchOtherDevice(modelData.id)
                }
            }

            // Flexible Spacer (pushes controls to the right)
            Item {
                Layout.fillWidth: true
            }

            // TODO:  Settings Button ---
            // ToolButton {
            //     text: "⚙"
            //     font.pixelSize: 14
            //     implicitWidth: 34
            //     implicitHeight: parent.height
            //     onClicked: settingsDialog.open()
            // }

            // --- Window Control Buttons ---
            Row {
                Layout.fillHeight: true
                spacing: 0

                // Minimize
                ToolButton {
                    id: minBtn
                    text: "_"
                    font.pixelSize: 11
                    implicitWidth: 34
                    implicitHeight: titleBar.height
                    onClicked: mainWindow.showMinimized()
                }

                // Maximize / Restore
                ToolButton {
                    id: maxBtn
                    text: mainWindow.visibility === Window.Maximized ? "❐" : "☐"
                    font.pixelSize: 12
                    implicitWidth: 34
                    implicitHeight: titleBar.height
                    onClicked: {
                        if (mainWindow.visibility === Window.Maximized)
                            mainWindow.showNormal()
                        else
                            mainWindow.showMaximized()
                    }
                }

                // Close Button with standard accent color on hover
                ToolButton {
                    id: closeBtn
                    text: "✕"
                    font.pixelSize: 12
                    implicitWidth: 34
                    implicitHeight: titleBar.height-2

                    background: Rectangle {
                        color: closeBtn.hovered ? "#e81123" : "transparent"
                    }

                    contentItem: Text {
                        text: closeBtn.text
                        font: closeBtn.font
                        color: closeBtn.hovered ? "white" : theme.appDarkGray
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: Qt.quit()
                }
            }
        }
    }


    // -------------------------------------------------------------------------
    // Main Content Area
    // -------------------------------------------------------------------------
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.margins: 16
        spacing: 12

        anchors {
            top: titleBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Rectangle {
            id: unreachableWarning
            Layout.fillWidth: true
            visible: deviceStatus.status === DeviceStatus.DeviceUnreachable
            color: "#FFF2AC" // Gentle warning yellow (or use your theme color)

            // Binding height to the inner label + padding
            implicitHeight: warningText.implicitHeight + 16

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#E6C200" // Subtle accent bottom border
            }

            Text {
                id: warningText
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    leftMargin: 16
                    rightMargin: 16
                    topMargin: 8
                }
                text: qsTr("The phone is not reachable. If it's nearby, try unlocking it and checking the KDE Connect App. " +
                           "Sending messages is unavailable and the conversations shown below may not be up-to-date." +
                           "If it's unreachable because you've replaced the phone, you should go into KDE Connect and " +
                           "unpair the device.")
                wrapMode: Text.Wrap
                font.pixelSize: 12
                color: "#5C4900"
            }
        }

        SplitView {
            id: split
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Name will be null in all the "totally failed to work" cases.  If it's not null,
            // it means that we're pretty sure the phone is just offline and users can use the cached
            // data until it comes back.
            visible: deviceStatus.deviceName !== ""

            ConversationList {
                id: conversationList
                Layout.fillWidth: true
                Layout.fillHeight: true
                SplitView.preferredWidth: windowSettings.conversationListWidth
                SplitView.minimumWidth: 100 * Screen.devicePixelRatio

                onWidthChanged: {
                    if (width > 0) {
                        windowSettings.conversationListWidth = width
                    }
                }
            }

            MessageList {
                SplitView.minimumWidth: 100 * Screen.devicePixelRatio
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        RowLayout {
            id: error
            visible: deviceStatus.deviceName === ""
            Layout.fillWidth: true

            Image {
                height: 48
                width: 48
                source: "qrc:/icons/error.svg"
            }
            Text {
                visible: deviceStatus.status === DeviceStatus.DaemonNotRunning
                Layout.fillWidth: true
                text: qsTr("The KDE Daemon is not running (or at least it's not responding to commands sent " +
                           "to it via dbus).  Perhaps you just need to install it?")
                wrapMode: Text.Wrap
            }
            Text {
                visible: deviceStatus.status === DeviceStatus.DaemonHung
                Layout.fillWidth: true
                text: qsTr("The KDE Daemon doesn't appear to be responding to messages; it might be hung. " +
                           "You might try kicking it with `pkill kdeconnected`.  It can be sufficiently " +
                           "hung up that it won't even respond to `pkill -9...`, in which case reboot.")
                wrapMode: Text.Wrap
            }
            Text {
                visible: deviceStatus.status === DeviceStatus.NoSmsDevice
                Layout.fillWidth: true
                text: qsTr("No phone (or SMS-capable device) could be found. Have you paired your phone " +
                           "with KDE Connect? If not, do that. Otherwise, is it maybe just offline? " +
                           "Unlock the phone and check if it's connected with KDE Connect by opening " +
                           "the KDE Connect app on the phone.")
                wrapMode: Text.Wrap
            }
            Text {
                visible: deviceStatus.status === DeviceStatus.DeviceMissing
                Layout.fillWidth: true
                text: qsTr("The phone this process is configured to talk to is no longer paired with KDE. " +
                           "Maybe you should remove this startup shortcut?  Or perhaps re-pair your phone?")
                wrapMode: Text.Wrap
            }
        }

        Item {
            id: errorFiller
            Layout.fillHeight: true
            visible: deviceStatus.deviceName === ""
        }
    }

    // Top
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 6
        cursorShape: Qt.SizeVerCursor
        onPressed: mainWindow.startSystemResize(Qt.TopEdge)
    }

    // Bottom
    MouseArea {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        cursorShape: Qt.SizeVerCursor
        onPressed: mainWindow.startSystemResize(Qt.BottomEdge)
    }

    // Left
    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        cursorShape: Qt.SizeHorCursor
        onPressed: mainWindow.startSystemResize(Qt.LeftEdge)
    }

    // Right
    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 6
        cursorShape: Qt.SizeHorCursor
        onPressed: mainWindow.startSystemResize(Qt.RightEdge)
    }

    // Top-Left
    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        width: 12
        height: 12
        cursorShape: Qt.SizeFDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
        z: 9999
    }

    // Top-Right
    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        width: 12
        height: 12
        cursorShape: Qt.SizeBDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.TopEdge | Qt.RightEdge)
        z: 9999
    }

    // Bottom-Left
    MouseArea {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: 12
        height: 12
        cursorShape: Qt.SizeBDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
        z: 9999
    }

    // Bottom-Right
    MouseArea {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 12
        height: 12
        cursorShape: Qt.SizeFDiagCursor
        onPressed: mainWindow.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
        z: 9999
    }
}