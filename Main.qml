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
    // property DeviceStatus deviceStatus: deviceStatus
    // property ConversationListModel conversations: conversations

    Component.onCompleted: {
        console.log("status =", deviceStatus.status)
        console.log("enum =", DeviceStatus.NoSmsDevice)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Rectangle {
            visible: deviceStatus.status === DeviceStatus.DaemonNotRunning

            Layout.fillWidth: true
            Layout.preferredHeight: row.implicitHeight + 16   // padding top+bottom
            color: "salmon"
            border.color: "red"
            border.width: 2
            radius: 6

            Label {
                text: "KDE Connect is not installed or running"
                color: "white"
                padding: 8
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.fill: parent
            }
        }

        Rectangle {
            visible: deviceStatus.status === DeviceStatus.DaemonHung

            Layout.fillWidth: true
            Layout.preferredHeight: row.implicitHeight + 16   // padding top+bottom
            color: "salmon"
            border.color: "red"
            border.width: 2
            radius: 6

            RowLayout {
                id: row
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

        Rectangle {
            visible: deviceStatus.status === DeviceStatus.NoSmsDevice
            Layout.fillWidth: true
            Layout.preferredHeight: row.implicitHeight + 16   // padding top+bottom
            color: "salmon"
            border.color: "red"
            border.width: 2
            radius: 6

            RowLayout {
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

        Rectangle {
            visible: deviceStatus.status === DeviceStatus.DeviceUnreachable || deviceStatus.status === DeviceStatus.DeviceReady
            radius: 4
            color: "#333333"
            Layout.fillWidth: true
            Layout.preferredHeight: row.implicitHeight + 16   // padding top+bottom
            border.color: "black"
            border.width: 2

            RowLayout {
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

        // ------------------------------------------------------------
        // Conversation list
        // ------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#f5f5f5"

            ScrollView {
                anchors.fill: parent
                clip: true

                ListView {
                    id: conversationList
                    anchors.fill: parent
                    anchors.margins: 8
                    model: conversations
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
}
