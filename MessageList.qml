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

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            spacing: 6

            Avatar {
                size: 35
                participants: messageListModel.avatarData
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                Layout.fillWidth: true
                text: messageListModel.participants
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }
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
                        color: object.isIncoming ? "#e8f4ff" : "#2222ff"

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
                                color: object.isIncoming ? "#222222" : "#ffffff"

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

    // Send area
    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        height: 50

        // -----------------------------
        // Text input
        // -----------------------------
        TextArea {
            id: inputField
            Layout.fillWidth: true
            Layout.preferredWidth: inputField.text.length > 0 ? parent.width * 0.80 : parent.width
            wrapMode: TextArea.Wrap
            placeholderText: "Type a message"
            readOnly: messageListModel.isSending
            text: messageListModel.draftText

            onTextChanged: {
                messageListModel.draftText = text
            }

            Keys.onReturnPressed: {
                if (!messageListModel.isSending && inputField.text.length > 0) {
                    messageListModel.sendMessage(inputField.text)
                }
            }
        }

        // -----------------------------
        // Send button (paper airplane)
        // -----------------------------
        Item {
            id: sendButton
            visible: inputField.text.length > 0
            enabled: inputField.text.length > 0 && !messageListModel.isSending
            width: 40
            height: 40

            // Circular background
            Rectangle {
                id: bg
                anchors.fill: parent
                radius: width / 2
                color: enabled ? "#4a90e2" : "#aaaaaa"
            }

            // Triangle (paper airplane)
            Canvas {
                id: triangle
                anchors.centerIn: parent
                width: 20
                height: 20

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.fillStyle = "white";

                    ctx.beginPath();
                    ctx.moveTo(0, height);
                    ctx.lineTo(width, height / 2);
                    ctx.lineTo(0, 0);
                    ctx.closePath();
                    ctx.fill();
                }
            }

            // Spin animation during send
            RotationAnimator {
                target: sendButton
                running: messageListModel.isSending
                from: 0
                to: 360
                duration: 800
                loops: Animation.Infinite

                onRunningChanged: {
                    if (!running) {
                        sendButton.rotation = 0
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: !messageListModel.isSending
                onClicked: {
                    messageListModel.sendMessage(inputField.text)
                }
            }
        }
    }
}
