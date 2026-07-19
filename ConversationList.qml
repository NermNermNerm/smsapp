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
                focus: true
                anchors.fill: parent
                anchors.margins: 8
                model: conversationListModel
                clip: true

                onCurrentItemChanged: {
                    messageListModel.setConversationID(currentItem ? currentItem.conversationID : -1)
                    console.log("Set conversation id to " + (currentItem ? currentItem.conversationID : -1))
                }

                highlight: Rectangle {
                    color: "#3498db"
                    radius: 4
                    Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
                }
                highlightFollowsCurrentItem: true

                delegate: ItemDelegate {
                    id: delegateItem
                    width: ListView.view.width
                    height: content.implicitHeight + 16

                    topPadding: 8
                    bottomPadding: 8
                    leftPadding: 12
                    rightPadding: 12

                    property int conversationID: model.object.conversationID

                    onClicked: {
                        conversationList.currentIndex = index;
                    }

                    background: Rectangle {
                        color: delegateItem.ListView.isCurrentItem ? "transparent" : "white"
                        border.color: delegateItem.ListView.isCurrentItem ? "transparent" : "#e0e0e0"
                        radius: 4

                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    contentItem: RowLayout {
                        Avatar {
                            participants: object.avatarData
                        }

                        ColumnLayout {
                            id: content
                            spacing: 4
                            Layout.fillWidth: true

                            Label {
                                Layout.fillWidth: true
                                text: object.participants
                                font.bold: object.isUnread
                                font.pointSize: 14
                                elide: Text.ElideRight

                                color: delegateItem.ListView.isCurrentItem ? "white" : "#222222"

                                HoverHandler { id: hh2 }
                                ToolTip.visible: hh2.hovered && truncated
                                ToolTip.text: object.participants
                                ToolTip.delay: 2000
                            }

                            Label {
                                Layout.fillWidth: true
                                text: object.latestMessageBody
                                font.italic: object.isLatestDraft
                                font.pointSize: 12
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                                elide: Text.ElideRight

                                color: delegateItem.ListView.isCurrentItem ? "#e0e0e0" : "#666666"

                                HoverHandler { id: hh }
                                ToolTip.visible: hh.hovered && truncated
                                ToolTip.text: object.latestMessageBody
                                ToolTip.delay: 2000
                            }
                        }

                        Text {
                            text: object.shortFriendlyDate
                            font.pointSize: 10

                            color: delegateItem.ListView.isCurrentItem ? "#d0d0d0" : "#999999"

                            HoverHandler { id: hhdate }
                            ToolTip.visible: hhdate.hovered
                            ToolTip.text: Qt.formatDateTime(object.date, Qt.DefaultLocaleLongDate)
                            ToolTip.delay: 2000
                        }
                    }
                }
            }
        }
    }
}