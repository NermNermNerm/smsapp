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

    AttachmentListModel {
        id: attachmentList
        attachments: root.attachments
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

            Item { visible: !root.isIncoming; Layout.fillWidth: true }

            Image {
                id: preview
                visible: isImage(model)
                source: model.fileUri
                fillMode: Image.PreserveAspectFit
                // 1. Leave sourceSize ALONE. This allows QML to populate sourceSize.width
                // and sourceSize.height with the true encoded dimensions of the file.

                // 2. Calculate the native aspect ratio directly from the file data
                readonly property real aspectRatio: sourceSize.height > 0 ? (sourceSize.width / sourceSize.height) : 1.0

                // 3. Set a safe maximum cap that doesn't rely on the ColumnLayout's width.
                // (Tip: If you want this to be responsive, change 400 to something outside the
                // layout chain, like 'mainWindow.width * 0.5')
                property real maxPreviewWidth: 400

                // 4. Let the image's own dimensions dictate the layout bounds!
                // It will perfectly scale down to match its aspect ratio, but never upscale tiny images.
                Layout.preferredWidth: Math.min(sourceSize.width, maxPreviewWidth)
                Layout.preferredHeight: sourceSize.width > 0 ? (Layout.preferredWidth / aspectRatio) : 0

                Layout.fillWidth: false
                Layout.fillHeight: false

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onDoubleClicked: attachmentList.openImage(model.index)
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
