import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Sms 1.0   // AttachmentListModel

Column {
    id: root

    property var attachments

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

    Repeater {
        model: attachmentList

        RowLayout {
            spacing: 12

            Text {
                // yields "pdf file - 147kb"
                text: qsTr("%1 file — %2").arg(model.extension).arg(humanSize(model.size))
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            Button {
                implicitWidth: implicitContentWidth+2
                icon.source: "qrc:/icons/download.svg"
                display: AbstractButton.IconOnly
                icon.width: 20
                icon.height: 20
                ToolTip.visible: hovered
                ToolTip.text: "Download"
                ToolTip.delay: 400
                onClicked: attachmentList.download(model.index)
            }
        }
    }
}
