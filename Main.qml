import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Component.onCompleted: {
        console.log("status =", deviceStatus.status)
        console.log("enum =", DeviceStatus.NoSmsDevice)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        StatusBar {
            Layout.fillWidth: true
        }

        ConversationList {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
