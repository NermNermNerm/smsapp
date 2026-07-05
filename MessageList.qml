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
        clip: true

        ListView {
            id: messageList
            model: messageListModel
            spacing: 12
            clip: true

            delegate: ColumnLayout {
                width: ListView.view.width
                height: contentLayout.implicitHeight
                spacing: 4

                // ---- Date label ----
                Label {
                    Layout.fillWidth: true
                    text: Qt.formatDateTime(object.date, "dddd, MMMM d, yyyy")
                    font.pointSize: 10
                    color: "#666666"
                    horizontalAlignment: Text.AlignHCenter
                }

                // ---- Message bubble ----
                Rectangle {
                    radius: 8
                    color: object.isIncoming
                           ? "#e8f4ff"   // very light blue
                           : "#cce8ff"   // baby blue

                    Layout.alignment: object.isIncoming
                                      ? Qt.AlignLeft
                                      : Qt.AlignRight

                    Layout.leftMargin: object.isIncoming ? 12 : 40
                    Layout.rightMargin: object.isIncoming ? 40 : 12

                    // Bubble content: layout-based, no anchors
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.margins: 8

                        Text {
                            text: object.body
                            wrapMode: Text.Wrap
                            elide: Text.ElideNone
                            font.pointSize: 14
                            color: "#222222"
                            Layout.fillWidth: true
                            verticalAlignment: Text.AlignTop
                        }
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
