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
                source: "data:" + model.mimeType + ";base64," + model.base64
                fillMode: Image.PreserveAspectFit

                // Now these layout properties will actually work!
                //Layout.maxWidth: 200 // Or whatever max size fits your bubble design
                //Layout.fillWidth: false

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onDoubleClicked: attachmentList.openImage(model.index)
                }
            }

            Text {
                visible: !isImage(model)
                // yields "pdf file - 147kb"
                text: qsTr("%1 file — %2").arg(model.extension).arg(humanSize(model.base64.length*3/4))
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
