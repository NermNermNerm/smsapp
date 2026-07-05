import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

ColumnLayout {
    id: root
    spacing: 8
    Layout.fillWidth: true
    Layout.fillHeight: true

    // ============================
    // Header placeholder
    // ============================
    Rectangle {
        Layout.fillWidth: true
        height: 40
        color: "#dddddd"

        Text {
            anchors.centerIn: parent
            text: "Conversation Header"
            font.bold: true
        }
    }

    // ============================
    // Message list
    // ============================
    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: messageList
            model: messageListModel
            spacing: 12

            delegate: Rectangle {
                width: messageList.width - 32
                radius: 8
                color: object.isIncoming ? "#e8f4ff" : "#cce8ff"
                height: contentLayout.implicitHeight + 24

                ColumnLayout {
                    id: contentLayout
                    x: 16
                    y: 12
                    width: parent.width - 32 // 16px left + 16px right padding
                    Text {
                        text: object.body
                        wrapMode: Text.Wrap
                        font.pointSize: 14
                        color: "#222222"
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    // ============================
    // Footer placeholder
    // ============================
    Rectangle {
        Layout.fillWidth: true
        height: 50
        color: "#dddddd"

        Text {
            anchors.centerIn: parent
            text: "Compose Area Placeholder"
        }
    }
}
