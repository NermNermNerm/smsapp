import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import Sms 1.0

ApplicationWindow {
    id: mainWindow
    width: 940
    height: 480
    visible: true
    title: qsTr("Texts – %1").arg(deviceStatus.deviceName)

    Settings {
        id: windowSettings
        category: "Window"

        // Property aliases bind them directly to the window's actual properties
        property alias x: mainWindow.x
        property alias y: mainWindow.y
        property alias width: mainWindow.width
        property alias height: mainWindow.height
    }

    Component.onCompleted: {
        // If the saved x/y coordinate sits completely off the active screen layout, center the window
        if (mainWindow.x < screen.virtualX || mainWindow.x > (screen.virtualX + screen.width - 100)) {
            mainWindow.x = screen.virtualX + (screen.width - mainWindow.width) / 2
        }
        if (mainWindow.y < screen.virtualY || mainWindow.y > (screen.virtualY + screen.height - 100)) {
            mainWindow.y = screen.virtualY + (screen.height - mainWindow.height) / 2
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        StatusBar {
            Layout.fillWidth: true
        }

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
}
