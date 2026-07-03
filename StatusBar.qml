import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

Item {
    id: root
    implicitHeight: cols.implicitHeight

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        id: cols

        // DaemonNotRunning
        Rectangle {
            visible: deviceStatus.status === DeviceStatus.DaemonNotRunning
            Layout.fillWidth: true
            Layout.preferredHeight: row1.implicitHeight + 16
            color: "salmon"
            border.color: "red"
            border.width: 2
            radius: 6

            Label {
                id: row1
                text: "KDE Connect is not installed or running"
                color: "white"
                padding: 8
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
        }

        // DaemonHung
        Rectangle {
            visible: deviceStatus.status === DeviceStatus.DaemonHung
            Layout.fillWidth: true
            Layout.preferredHeight: row2.implicitHeight + 16
            color: "salmon"
            border.color: "red"
            border.width: 2
            radius: 6

            RowLayout {
                id: row2
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                Rectangle {
                    color: "red"
                    width: 14
                    height: 14
                    radius: 7
                }

                Label {
                    text: "KDE Connect daemon is not responsive"
                    color: "white"
                    Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                }

                Button {
                    text: "Reset KDE"
                    onClicked: deviceStatus.rebootDaemon()
                }
            }
        }

        // NoSmsDevice
        Rectangle {
            visible: deviceStatus.status === DeviceStatus.NoSmsDevice
            Layout.fillWidth: true
            Layout.preferredHeight: row3.implicitHeight + 16
            color: "salmon"
            border.color: "red"
            border.width: 2
            radius: 6

            RowLayout {
                id: row3
                spacing: 2
                anchors.fill: parent
                anchors.margins: 8

                Rectangle {
                    color: "red"
                    width: 14
                    height: 14
                    radius: 7
                }

                Label {
                    text: "No phone is connected with KDE Connect"
                    color: "black"
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // DeviceUnreachable / DeviceReady
        Rectangle {
            visible: deviceStatus.status === DeviceStatus.DeviceUnreachable ||
                     deviceStatus.status === DeviceStatus.DeviceReady
            radius: 4
            color: "#333333"
            Layout.fillWidth: true
            Layout.preferredHeight: row4.implicitHeight + 16
            border.color: "black"
            border.width: 2

            RowLayout {
                id: row4
                anchors.fill: parent
                anchors.margins: 8
                spacing: 10

                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    color: deviceStatus.status === DeviceStatus.DeviceReady
                           ? "#00c853"
                           : "#ffca28"

                    Label {
                        visible: deviceStatus.status === DeviceStatus.DeviceUnreachable
                        text: "!"
                        anchors.centerIn: parent
                        font.pixelSize: 10
                        color: "black"
                    }
                }

                Label {
                    text: deviceStatus.preferredDeviceName
                    color: "white"
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }
            }
        }
    }
}
