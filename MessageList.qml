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
    ListView {
        id: messageList
        model: messageListModel
        spacing: 12
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        verticalLayoutDirection: ListView.BottomToTop

        ScrollBar.vertical: ScrollBar {
            visible: true
            active: true // Keeps the scrollbar visible during scroll activity
        }

        delegate: Item {
            id: delegateRoot
            width: messageList.width
            height: bubble.height

            // Helper property to calculate the true, unwrapped width of the text
            property real maxBubbleContentWidth: messageList.width - 64

            ColumnLayout {
                id: bubble
                width: parent.width

                Text {
                    visible: object.isDisplayDateVisible
                    horizontalAlignment: Text.AlignHCenter
                    text: object.displayDate
                    color: "#444444"
                    Layout.fillWidth: true
                }

                RowLayout {
                    Item { visible: !object.isIncoming; Layout.fillWidth: true }

                    Rectangle {
                        width: contentLayout.width + 32
                        height: contentLayout.implicitHeight + 24
                        radius: 8
                        color: object.isIncoming ? "#e8f4ff" : "#cce8ff"

                        Item {
                            id: contentLayout
                            x: 16
                            y: 12

                            // If the text naturally fits on one line, use its implicitWidth.
                            // Otherwise, cap it at the maximum allowed width.
                            width: Math.min(textBlock.implicitWidth, delegateRoot.maxBubbleContentWidth)

                            // Pass the layout's height down from the text element
                            property real implicitHeight: textBlock.implicitHeight

                            Text {
                                id: textBlock
                                text: object.body
                                color: "#222222"

                                // Stretch to fill the Item parent width calculated above
                                width: parent.width

                                // Only wrap if the text is actually forced to take up the max width
                                wrapMode: textBlock.implicitWidth > delegateRoot.maxBubbleContentWidth ? Text.Wrap : Text.NoWrap

                                horizontalAlignment: object.isIncoming ? Text.AlignLeft : Text.AlignRight
                            }
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
