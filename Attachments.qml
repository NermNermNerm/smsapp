import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1
import Sms 1.0   // AttachmentListModel

ColumnLayout {
    id: root
    width: parent.width
    spacing: 6

    property var attachments
    property bool isIncoming
    // 1. Snag the max width property passed from the message list container.
    // We add a fallback (400) just in case it's instantiated somewhere else without it.
    property real maxImageWidth: 400

    AttachmentListModel {
        id: attachmentList
        attachments: root.attachments
        messagesHandler: deviceStatus.handler
    }

    // Convert raw byte count → "128 KB", "2.3 MB", etc.
    function humanSize(bytes) {
        if (bytes < 1024)
            return qsTr("<1kb");
        if (bytes < 1024 * 1024)
            return qsTr("%1kb").arg(Math.round(bytes / 1024));
        if (bytes < 1024 * 1024 * 1024)
            return qsTr("%1mb").arg((bytes / (1024 * 1024)).toFixed(1));
        return qsTr("%1gb").arg((bytes / (1024 * 1024 * 1024)).toFixed(1));
    }

    function isImage(a) {
        return a.mimeType.startsWith("image/");
    }

    Repeater {
        model: attachmentList

        RowLayout {
            spacing: 12
            Layout.fillWidth: true
            Layout.leftMargin: 8

            Item { visible: !root.isIncoming; Layout.fillWidth: true }

            Image {
                id: preview
                visible: isImage(model)
                source: model.fileUri === ""
                        ? ("data:" + model.mimeType + ";base64," + model.thumbnail)
                        : model.fileUri;
                fillMode: Image.PreserveAspectFit
                Layout.fillWidth: false
                Layout.preferredWidth: Math.min(implicitWidth, root.maxImageWidth)
                Layout.preferredHeight: implicitWidth > 0
                                        ? (Math.min(implicitWidth, root.maxImageWidth) * (implicitHeight / implicitWidth))
                                        : 0

                onStatusChanged: {
                    if (status === Image.Ready && model.fileUri === "") {
                        // Thumbnail just became visible → request full attachment
                        attachmentList.requestFullAttachment(model.index)
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onDoubleClicked: attachmentList.open(model.index)
                }
            }

            Text {
                visible: !isImage(model)
                // yields "pdf file - 147kb"
                text: qsTr("%1 file — %2").arg(model.extension).arg(humanSize(model.size))
                wrapMode: Text.Wrap
            }

            FileDialog {
                id: saveDialog
                title: qsTr("Save Attachment")
                fileMode: FileDialog.SaveFile
                currentFile: "attachment." + model.extension;
                nameFilters: [
                    qsTr("%1 files (*.%1)").arg(model.extension),
                    qsTr("All files (*.*)")
                ]

                onAccepted: attachmentList.saveToPath(model.index, saveDialog.file)
            }

            Button {
                visible: !isImage(model)
                implicitWidth: implicitContentWidth+2
                icon.source: "qrc:/icons/download.svg"
                display: AbstractButton.IconOnly
                icon.width: 20
                icon.height: 20
                ToolTip.visible: hovered
                ToolTip.text: "Download"
                ToolTip.delay: 400
                onClicked: {
                    saveDialog.currentFile = "attachment." + model.extension;
                    saveDialog.open();
                }
            }
        }
    }
}
