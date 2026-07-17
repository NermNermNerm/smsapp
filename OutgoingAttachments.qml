import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Sms 1.0

Item {
    id: root
    property OutgoingAttachmentListModel model

    // =========================================================================
    // Dynamic Layout & Visibility
    // =========================================================================
    // 1. Only visible when there are active attachments to show
    visible: model && !model.isEmpty

    // 2. Drive height dynamically. If empty, it collapses to 0.
    implicitHeight: (model && !model.isEmpty) ? 116 : 0
    Layout.fillWidth: true

    // 3. Keep it from overflowing when collapsing
    clip: true

    // 4. Smoothly slide the ribbon open/closed
    Behavior on implicitHeight {
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    ListView {
        id: view
        anchors.fill: parent
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        orientation: ListView.Horizontal
        spacing: 12
        model: root.model
        clip: false // Let the hover/close buttons pop out of the boundary slightly

        delegate: Item {
            id: delegateRoot
            height: 100
            // Calculate width dynamically based on the image's physical aspect ratio
            width: {
                // Fallback default if image hasn't loaded yet
                if (previewImage.implicitWidth <= 0 || previewImage.implicitHeight <= 0) {
                    return 100;
                }

                let ratio = previewImage.implicitWidth / previewImage.implicitHeight;
                let calculatedWidth = ratio * 100; // 100 is our fixed height

                // Clamp it: Don't let wide images take over the whole screen (max 200px),
                // and don't let tall images shrink into a tiny sliver (min 60px).
                return Math.min(Math.max(calculatedWidth, 60), 300);
            }

            // =================================================================
            // The Preview Card
            // =================================================================
            Rectangle {
                id: card
                anchors.fill: parent
                radius: 8
                color: "#f5f5f7"
                border.color: "#e5e5ea"
                border.width: 1
                clip: true // Ensure image corners are clipped nicely

                // 1. Image Preview (Attempts to load the source)
                AnimatedImage {
                    id: previewImage
                    anchors.fill: parent
                    source: model.fileUri
                    fillMode: Image.PreserveAspectCrop

                    // The magic: If Qt can't decode it as an image, it transitions
                    // to Image.Error, and we seamlessly hide it!
                    visible: status === Image.Ready
                }

                // 2. Generic File Fallback (Shows if it's not a loadable image)
                ColumnLayout {
                    id: fallbackLayout
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4
                    visible: previewImage.status !== Image.Ready

                    // Generic Document Icon / Emoji
                    Text {
                        text: "📄"
                        font.pixelSize: 32
                        Layout.alignment: Qt.AlignHCenter
                    }

                    // Filename elided in the middle (ideal for files)
                    Text {
                        text: model.filename
                        font.pixelSize: 11
                        color: "#3a3a3c"
                        elide: Text.ElideMiddle
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                }
            }

            // =================================================================
            // Sleek Floating Delete Button
            // =================================================================
            Rectangle {
                id: deleteButton
                width: 22
                height: 22
                radius: 11

                // Positioned overlapping the top-right corner of the card
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: -6
                z: 10 // Force it to float on top of everything

                // Sleek iOS-style blur look: semi-transparent black that highlights red on hover
                color: deleteMouseArea.containsMouse ? "#ff3b30" : "#b3000000"
                border.color: "white"
                border.width: 1.5

                Text {
                    text: "×"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    anchors.centerIn: parent
                    anchors.verticalCenterOffset: -1 // Visually align the '×' vertically
                }

                MouseArea {
                    id: deleteMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.model.remove(index)
                }
            }
        }
    }
}