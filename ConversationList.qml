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
                anchors.rightMargin: 8
                model: conversationListModel
                clip: true

                onCurrentItemChanged: {
                    messageListModel.setConversationID(currentItem ? currentItem.conversationID : -1)
                    console.log("Set conversation id to " + (currentItem ? currentItem.conversationID : -1))
                }

                highlight: Rectangle {
                    color: "#3498db"
                    radius: 4
                }
                highlightFollowsCurrentItem: true
                highlightMoveDuration: 80

                delegate: ItemDelegate {
                    id: delegateItem
                    width: ListView.view.width
                    height: content.implicitHeight + 16

                    property int conversationID: model.object.conversationID

                    onClicked: {
                        conversationList.currentIndex = index;
                    }

                    background: Item {}

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
                                font.pointSize: 12
                                elide: Text.ElideRight

                                color: delegateItem.ListView.isCurrentItem ? "white" : "#222222"

                                HoverHandler { id: hh2 }
                                ToolTip.visible: hh2.hovered && truncated
                                ToolTip.text: object.participants
                                ToolTip.delay: 2000
                            }

                            Label {
                                Layout.fillWidth: true
                                text: object.latestMessageBody + "\n" // ensure there's always 2 lines
                                font.italic: object.isLatestDraft
                                font.pointSize: 10
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