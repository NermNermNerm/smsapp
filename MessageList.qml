import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

Item {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true

    OutgoingAttachmentListModel {
        id: outgoingAttachmentListModel
        all: messageListModel.draftAttachments

        onAllChanged: {
            // The guard check prevents infinite binding loops... hopefully.
            if (messageListModel.draftAttachments !== all) {
                messageListModel.draftAttachments = all;
            }
        }
    }

    function doSend()
    {
        let shouldDownscale = outgoingAttachmentListModel.isOversized && outgoingAttachmentListModel.isDownscaling && outgoingAttachmentListModel.isAbleToDownscale;
        messageListModel.sendMessage(inputField.text, outgoingAttachmentListModel.all, shouldDownscale)
    }


    // This is the actual visible content of the control
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        spacing: 8

        // vv----  Header area -----vv
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

        // vv----  List of messages  -----vv
        ListView {
            id: messageList
            model: messageListModel
            spacing: 12
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            verticalLayoutDirection: ListView.BottomToTop

            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar

                // Track
                background: Rectangle {
                    implicitWidth: 10
                    color: "#e0e0e0"
                    radius: 5

                    // Fades out smoothly when scrolling stops
                    opacity: verticalScrollBar.active ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                // Handle
                contentItem: Rectangle {
                    implicitWidth: 10
                    color: "#666666"
                    radius: 5

                    // Fades out smoothly when scrolling stops
                    opacity: verticalScrollBar.active ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }
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

                    Attachments {
                        isIncoming: object.isIncoming
                        attachments: object.attachments;

                        // Force the custom component to obey the bubble's layout width
                        Layout.fillWidth: true

                        // Pass a strict maximum image size down to the child elements.
                        maxImageWidth: delegateRoot.maxBubbleContentWidth
                        maxImageHeight: messageList.height > 0 ? messageList.height : 400
                    }

                    RowLayout {
                        visible: object.body !== ""

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

        // vv--------- Send area starts here -----------vv
        OutgoingAttachments {
            model: outgoingAttachmentListModel;
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: (inputField.text.length > 0 || !outgoingAttachmentListModel.isEmpty) ? 8 : 0 // Collapses layout spacing when hidden
            height: 50

            // Smoothly animate the spacing collapse for the button
            Behavior on spacing { NumberAnimation { duration: 200 } }

            // Outgoing message text input area
            TextArea {
                id: inputField
                Layout.fillWidth: true // Automatically claims all leftover space in the row
                wrapMode: TextArea.Wrap
                placeholderText: "Type a message"
                readOnly: messageListModel.isSending
                text: messageListModel.draftText

                leftPadding: 12
                rightPadding: 12
                topPadding: 10
                bottomPadding: 10

                background: Rectangle {
                    color: "#e8f4ff"
                    radius: 8
                    border.width: 0
                }

                onTextChanged: {
                    messageListModel.draftText = text
                }

                Keys.onReturnPressed: (event) => {
                    if (event.modifiers !== 0) { // If shift, let it through
                        event.accepted = false;
                    }
                    else {
                        if (sendButton.enabled) {
                            root.doSend()
                        }
                        event.accepted = true;
                    }
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_V && event.modifiers & Qt.ControlModifier) {
                        let url = messageListModel.turnClipboardIntoAttachment();
                        if (url.toString() !== "") {
                            outgoingAttachmentListModel.add(url)
                            event.accepted = true
                        }
                    }
                }
            }

            Item {
                id: sendButton
                enabled: (inputField.text.length > 0 || !outgoingAttachmentListModel.isEmpty)
                         && (!outgoingAttachmentListModel.isOversized || (outgoingAttachmentListModel.isAbleToDownscale && outgoingAttachmentListModel.isDownscaling))
                         && !messageListModel.isSending
                height: 40

                Layout.preferredWidth: (inputField.text.length > 0 || !outgoingAttachmentListModel.isEmpty) ? 40 : 0
                Layout.preferredHeight: 40
                opacity: (inputField.text.length > 0 || !outgoingAttachmentListModel.isEmpty) ? 1.0 : 0.0
                clip: true

                Behavior on Layout.preferredWidth {
                    NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }

                // Circular background
                Rectangle {
                    id: bg
                    anchors.fill: parent
                    radius: width / 2
                    color: sendButton.enabled ? "#2222ff": "#bbbbbb"
                }

                // Triangle (paper airplane)
                Canvas {
                    id: triangle
                    anchors.centerIn: parent
                    width: 20
                    height: 20

                    scale: messageListModel.isSending ? 0.0 : 1.0
                    opacity: messageListModel.isSending ? 0.0 : 1.0

                    Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                    Behavior on opacity { NumberAnimation { duration: 150 } }

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);
                        ctx.strokeStyle = "white";
                        ctx.lineWidth = 1.5;
                        ctx.lineJoin = "round"

                        ctx.beginPath();
                        ctx.moveTo(width*.4, height/2);
                        ctx.lineTo(0, height/2-1);
                        ctx.lineTo(0, 0);
                        ctx.lineTo(width, height / 2);
                        ctx.lineTo(0, height);
                        ctx.lineTo(0, height/2+1);
                        ctx.closePath();

                        ctx.stroke();
                    }
                }

                BusyIndicator {
                    id: loadingSpinner
                    anchors.centerIn: parent
                    width: 30
                    height: 30

                    running: messageListModel.isSending
                    scale: running ? 1.0 : 0.0
                    opacity: running ? 1.0 : 0.0

                    Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }
                    Behavior on opacity { NumberAnimation { duration: 150 } }

                    contentItem: Canvas {
                        id: spinnerCanvas
                        width: parent.width
                        height: parent.height

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.strokeStyle = "white";
                            ctx.lineWidth = 3;
                            ctx.lineCap = "round";

                            ctx.beginPath();
                            ctx.arc(width / 2, height / 2, (width / 2) - ctx.lineWidth, 0, Math.PI * 1.5);
                            ctx.stroke();
                        }

                        RotationAnimator {
                            target: spinnerCanvas
                            running: loadingSpinner.running
                            from: 0
                            to: 360
                            duration: 1000
                            loops: Animation.Infinite
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: !messageListModel.isSending
                    onClicked: root.doSend()
                }
            }
        }
    }

    // Global Drop Area
    DropArea {
        id: globalDropArea
        anchors.fill: parent // Fills the root Item perfectly
        z: 10                // Keeps it sitting on top of everything during drag events

        function validateDrag(dragEvent) {
            if (!dragEvent.hasUrls || messageListModel.isSending) {
                dragEvent.accepted = false;
                return;
            }

            for (let i = 0; i < dragEvent.urls.length; i++) {
                let urlStr = dragEvent.urls[i].toString();
                // If any URL is not a local file, reject the entire drag
                if (!urlStr.startsWith("file://")) {
                    dragEvent.accepted = false;
                    return;
                }
            }

            dragEvent.accepted = true;
        }

        onEntered: (drag) => validateDrag(drag)
        onPositionChanged: (drag) => validateDrag(drag)

        onDropped: (drop) => {
            if (messageListModel.isSending) {
                return;
            }

            for (let i = 0; i < drop.urls.length; i++) {
               let urlStr = drop.urls[i].toString();
               if (urlStr.startsWith("file://")) {
                   outgoingAttachmentListModel.add(drop.urls[i]);
               }
            }
        }
    }
}