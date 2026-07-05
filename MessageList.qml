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

            delegate: Text {
                width: messageList.width
                text: object.body
                font.pointSize: 14
                color: "#222222"
                wrapMode: Text.Wrap
                elide: Text.ElideNone
                Layout.fillWidth: true
                verticalAlignment: Text.AlignTop
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
