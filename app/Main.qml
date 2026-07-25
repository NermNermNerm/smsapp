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
                width: 10
                height: 10
                radius: 5
                Layout.alignment: Qt.AlignVCenter
                color: deviceStatus.status === DeviceStatus.DeviceReady ? theme.statusOk
                      : deviceStatus.status === DeviceStatus.DeviceUnreachable ? theme.statusWarn
                      : theme.statusError
            }

            // --- Active Phone Name ---
            Label {
                text: deviceStatus.deviceName
                font.bold: true
                font.pixelSize: 13
                color: theme.titleBarButtonTextColor
                Layout.alignment: Qt.AlignVCenter
                Layout.rightMargin: 6
            }

            // --- Other Devices Switcher (Subtle Flat Style) ---
            Repeater {
                model: deviceStatus.otherDevices
                delegate: ToolButton {
                    text: modelData.name
                    font.pixelSize: 12
                    implicitHeight: 28
                    onClicked: deviceStatus.setPreferredDevice(modelData.id)
                }
            }

            // Flexible Spacer (pushes controls to the right)
            Item {
                Layout.fillWidth: true
            }

            // --- Settings Button ---
            ToolButton {
                text: "⚙"
                font.pixelSize: 14
                implicitWidth: 34
                implicitHeight: parent.height
                onClicked: settingsDialog.open()
            }

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
        anchors {
            top: titleBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        anchors.margins: 16
        spacing: 12

        SplitView {
            id: split
            Layout.fillWidth: true
            Layout.fillHeight: true

            ConversationList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                SplitView.preferredWidth: 300
                SplitView.minimumWidth: 200
                SplitView.maximumWidth: 500
            }

            MessageList {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    // Dummy settings dialog placeholder
    Dialog {
        id: settingsDialog
        modal: true
        title: "Settings"
        standardButtons: Dialog.Ok
        visible: false
    }
}