import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.platform 1.1 as Platform
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

            AnimatedImage {
                id: preview
                visible: isImage(model)
                source: model.fileUri === ""
                        ? ("data:" + model.mimeType + ";base64," + model.thumbnail)
                        : model.fileUri;
                fillMode: Image.PreserveAspectFit
                // 1. Let the component calculate its own base dimensions.
                // This triggers standard QML property listeners instantly when the asset initializes.
                width: model.isExpanded ? Math.min(implicitWidth, root.maxImageWidth) : 100
                height: implicitWidth > 0 ? (width * (implicitHeight / implicitWidth)) : 0

                // 2. Feed the clean, computed dimensions into the layout engine.
                // When 'width' or 'height' updates, it guarantees the layout engine
                // forces a refresh, completely bypassing the timing/caching bug.
                Layout.fillWidth: false
                Layout.preferredWidth: width
                Layout.preferredHeight: height

                Behavior on width {
                    // Prevent animating the initial pop-in when the image loads
                    enabled: preview.status === Image.Ready

                    NumberAnimation {
                        duration: 220 // Sweet spot for UI snappiness
                        easing.type: Easing.InOutQuad
                    }
                }

                onStatusChanged: {
                    if (status === Image.Ready && visible && model.fileUri === "") {
                        // Thumbnail just became visible → request full attachment
                        attachmentList.requestFullAttachment(model.index)
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onDoubleClicked: attachmentList.open(model.index)
                    onClicked: attachmentList.toggleExpanded(model.index)
                }

                // ==========================================
                // Simple Top-Right Busy Indicator
                // ==========================================
                BusyIndicator {
                    visible: parent.visible && model.isLoading
                    running: visible // Only runs CPU cycles when loading

                    // Strictly 20x20 visible footprint
                    implicitWidth: 20
                    implicitHeight: 20
                    padding: 0

                    // Float it neatly in the top-right corner of the image
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 6
                }
            }

            Text {
                visible: !isImage(model)
                text: qsTr("%1 file").arg(model.extension)
                wrapMode: Text.Wrap
            }

            Dialog {
                id: invalidLocationDialog
                parent: Overlay.overlay
                title: qsTr("Invalid Location")
                modal: true
                standardButtons: Dialog.Ok

                // Position globally in the center of the screen
                x: Math.round((parent.width - width) / 2)
                y: Math.round((parent.height - height) / 2)
                width: Math.min(340, parent.width - 40)

                property alias text: dialogLabel.text

                Label {
                    id: dialogLabel
                    width: parent.width
                    wrapMode: Text.Wrap
                }
            }

            Platform.FileDialog {
                id: saveDialog
                title: qsTr("Save Attachment")
                fileMode: Platform.FileDialog.SaveFile
                currentFile: "attachment." + model.extension;
                nameFilters: [
                    qsTr("%1 files (*.%1)").arg(model.extension),
                    qsTr("All files (*.*)")
                ]

                onAccepted: {
                    var fileUrlStr = saveDialog.file.toString()

                    // 1. Check if it's a local file protocol
                    if (!fileUrlStr.startsWith("file://")) {
                        invalidLocationDialog.text = qsTr("The selected location is not a local file.\nPlease choose a local folder.")
                        invalidLocationDialog.open()
                        return
                    }

                    // 2. Strip "file://" (7 characters) to get the raw path
                    // On Linux: "file:///home/user/file" becomes "/home/user/file"
                    var localPath = fileUrlStr.substring(7)

                    // 3. Decode URL characters (e.g. convert "%20" back to standard spaces " ")
                    localPath = decodeURIComponent(localPath)

                    // 4. Send the clean, verified local path to C++
                    attachmentList.saveToPath(model.index, localPath)
                }
            }

            Button { // download
                visible: !isImage(model) && !model.isLoading && model.fileUri === ""
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

            BusyIndicator {
                visible: !isImage(model) && model.isLoading && model.fileUri === ""
                running: visible // Only runs CPU animation cycles when active on screen
                implicitWidth: 20
                implicitHeight: 20

                // Crucial: Strip style padding so the spinner expands to fill this box!
                padding: 0
            }

            Button { // open file
                visible: !isImage(model) && !model.isLoading && model.fileUri !== ""
                implicitWidth: implicitContentWidth+2
                icon.source: "qrc:/icons/open-file.svg"
                display: AbstractButton.IconOnly
                icon.width: 20
                icon.height: 20
                ToolTip.visible: hovered
                ToolTip.text: "Open"
                ToolTip.delay: 400
                onClicked: {
                    attachmentList.open(model.index)
                }
            }


            Button {
                visible: !isImage(model) && !model.isLoading && model.fileUri !== ""
                implicitWidth: implicitContentWidth+2
                icon.source: "qrc:/icons/open-folder.svg"
                display: AbstractButton.IconOnly
                icon.width: 20
                icon.height: 20
                ToolTip.visible: hovered
                ToolTip.text: "Open folder"
                ToolTip.delay: 400
                onClicked: {
                    // get rid of the trailing folder thing
                    Qt.openUrlExternally(model.fileUri.replace(/\/[^\/]*$/, ""))
                }
            }
        }
    }
}
