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
        ColumnLayout {
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
        ColumnLayout {
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
        // 3. Conversation list
        // ------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#f5f5f5"

            ListView {
                id: conversationList
                anchors.fill: parent
                anchors.margins: 8
                model: backend.conversationList
                clip: true

                delegate: Item {
                    width: ListView.view.width
                    height: content.implicitHeight + 12

                    ColumnLayout {
                        id: content
                        anchors {
                            left: parent.left
                            right: parent.right
                            margins: 6
                        }

                        // Participants
                        Text {
                            text: object.participants
                            font.bold: true
                            font.pointSize: 14
                            elide: Text.ElideRight
                        }

                        // Latest message
                        Text {
                            text: object.latestMessageBody
                            color: "#666"
                            font.pointSize: 12
                            elide: Text.ElideRight
                        }

                        // Timestamp
                        Text {
                            text: Qt.formatDateTime(object.date, "yyyy-MM-dd hh:mm")
                            color: "#999"
                            font.pointSize: 10
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            console.log("Clicked conversation thread:", object.threadID)
                            // Later: navigate to message view
                        }
                    }
                }
            }
        }
    }
}
