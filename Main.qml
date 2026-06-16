import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    // Bind the C++ instance to a typed QML property
    property SmsBackend backend: smsBackend

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ------------------------------------------------------------
        // 1. If we have NO primary device and status is NoPrimaryDevice:
        //    Show a placeholder + (later) a list of devices + reasons.
        // ------------------------------------------------------------
        Item {
            visible: backend.deviceName === "" &&
                     backend.deviceStatus === "No primary device"

            ColumnLayout {
                spacing: 6

                Label {
                    text: "No primary phone selected"
                    font.pixelSize: 20
                }

                Label {
                    text: backend.extendedStatus
                    wrapMode: Text.WordWrap
                }

                // Placeholder for future device list
                Rectangle {
                    width: parent.width
                    height: 80
                    color: "#33333333"
                    radius: 6

                    Label {
                        anchors.centerIn: parent
                        text: "Device list goes here"
                    }
                }
            }
        }

        // ------------------------------------------------------------
        // 2. If we HAVE a deviceName, show device info + status
        // ------------------------------------------------------------
        Item {
            visible: backend.deviceName !== ""

            ColumnLayout {
                spacing: 6

                Label {
                    text: backend.deviceName
                    font.pixelSize: 22
                }

                Label {
                    text: backend.deviceStatus
                    font.pixelSize: 16
                }

                Label {
                    text: backend.extendedStatus
                    wrapMode: Text.WordWrap
                    color: "#666"
                }
            }
        }

        // ------------------------------------------------------------
        // 3. Last message section
        // ------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 120
            radius: 8
            color: "#eeeeee"

            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                Label {
                    text: backend.lastSender !== "" ?
                          "Last message from " + backend.lastSender :
                          "No messages yet"
                    font.pixelSize: 16
                }

                Label {
                    text: backend.lastMessage
                    wrapMode: Text.WordWrap
                    visible: backend.lastMessage !== ""
                }
            }
        }
    }
}
