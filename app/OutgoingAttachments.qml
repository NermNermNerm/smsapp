import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Sms 1.0

Item {
    id: root
    property OutgoingAttachmentListModel model

    visible: model && !model.isEmpty

    // Drive height dynamically. Adds 40px if the angry banner needs to show.
    implicitHeight: {
        if (!model || model.isEmpty) return 0;
        let baseHeight = 116;
        let bannerHeight = (model.hasOversizedFiles) ? 40 : 0;
        return baseHeight + bannerHeight;
    }

    Layout.fillWidth: true
    clip: true

    Behavior on implicitHeight {
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Error message for oversized attachments
        Rectangle {
            id: angryBanner
            Layout.fillWidth: true
            Layout.preferredHeight: (root.model && root.model.isOversized) ? 40 : 0
            visible: Layout.preferredHeight > 0
            color: "#ff3b30"
            clip: true // Prevents text/checkbox spilling while animating open/closed

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    text: "Size limit exceeded"
                    color: "white"
                    font.bold: true
                    Layout.fillWidth: true

                    HoverHandler {
                        id: hoverGetter
                    }

                    ToolTip.visible: hoverGetter.hovered
                    ToolTip.delay: 400
                    ToolTip.text: "Because of limits imposed on us by Android, the total message size has to be less than 600k."
                }

                CheckBox {
                    id: downscaleCheck
                    text: "Downscale Image"

                    // Custom text styling so it stays readable on the red background
                    contentItem: Text {
                        text: downscaleCheck.text
                        color: downscaleCheck.enabled ? "white" : "#b3ffffff"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: downscaleCheck.indicator.width + downscaleCheck.spacing
                    }

                    // Only enable if there is exactly ONE image
                    enabled: root.model && root.model.isAbleToDownscale

                    // The ToolTip magic
                    ToolTip.visible: hovered
                    ToolTip.delay: 400
                    ToolTip.text: "If you are attaching a single image, we can reduce its resolution to get it under the size limit."

                    checked: root.model.isDownscaling
                    onToggled: root.model.isDownscaling = checked
                }
            }
        }

        // The Attachment Ribbon
        ListView {
            id: view
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Adjust margins slightly since the Layout is handling spacing now
            topMargin: 8
            bottomMargin: 8
            orientation: ListView.Horizontal
            spacing: 12
            model: root.model
            clip: false

            delegate: Item {
                id: delegateRoot
                height: 100

                width: {
                    if (previewImage.implicitWidth <= 0 || previewImage.implicitHeight <= 0) {
                        return 100;
                    }
                    let ratio = previewImage.implicitWidth / previewImage.implicitHeight;
                    let calculatedWidth = ratio * 100;
                    return Math.min(Math.max(calculatedWidth, 60), 300);
                }

                Rectangle {
                    id: card
                    anchors.fill: parent
                    radius: 8
                    color: "#f5f5f7"
                    border.color: "#e5e5ea"
                    border.width: 1
                    clip: true

                    AnimatedImage {
                        id: previewImage
                        anchors.fill: parent
                        source: model.fileUri
                        fillMode: Image.PreserveAspectCrop
                        visible: status === Image.Ready
                    }

                    ColumnLayout {
                        id: fallbackLayout
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4
                        visible: previewImage.status !== Image.Ready

                        Text {
                            text: "📄"
                            font.pixelSize: 32
                            Layout.alignment: Qt.AlignHCenter
                        }

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

                Rectangle {
                    id: deleteButton
                    width: 22
                    height: 22
                    radius: 11
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: -6
                    z: 10

                    color: deleteMouseArea.containsMouse ? "#ff3b30" : "#b3000000"
                    border.color: "white"
                    border.width: 1.5

                    Text {
                        text: "×"
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -1
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
}