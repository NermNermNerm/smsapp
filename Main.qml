import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        Text {
            text: "Device Status: " + smsBackend.deviceStatus
            font.pixelSize: 20
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#888"
        }

        Text {
            text: smsBackend.lastSender.length > 0
                  ? "Last message from: " + smsBackend.lastSender
                  : "No messages received yet"
            font.pixelSize: 16
        }

        Text {
            text: smsBackend.lastMessage
            wrapMode: Text.Wrap
            font.pixelSize: 14
        }
    }
}
