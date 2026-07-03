import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

Item {
    id: root

    Rectangle {
        anchors.fill: parent
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

                    MouseArea {
                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: {
                            console.log("Clicked conversation thread:", object.latestMessageBody)
                        }
                    }

                    ColumnLayout {
                        id: content
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 6

                        Label {
                            Layout.fillWidth: true
                            text: object.participants
                            font.bold: true
                            font.pointSize: 14
                            elide: Text.ElideRight

                            HoverHandler { id: hh2 }
                            ToolTip.visible: hh2.hovered && truncated
                            ToolTip.text: object.participants
                        }

                        Label {
                            Layout.fillWidth: true
                            text: object.latestMessageBody
                            color: "#666"
                            font.pointSize: 12
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight

                            HoverHandler { id: hh }
                            ToolTip.visible: hh.hovered && truncated
                            ToolTip.text: object.latestMessageBody
                        }

                        Text {
                            text: object.shortFriendlyDate
                            color: "#999"
                            font.pointSize: 10

                            HoverHandler { id: hhdate }
                            ToolTip.visible: hhdate.hovered
                            ToolTip.text: Qt.formatDateTime(object.date, Qt.DefaultLocaleLongDate)
                        }
                    }
                }
            }
        }
    }
}
