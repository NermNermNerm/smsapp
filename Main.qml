import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Sms 1.0

Window {
    width: 940
    height: 480
    visible: true
    title: qsTr("Hello World")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        StatusBar {
            Layout.fillWidth: true
        }

        SplitView {
            id: split
            Layout.fillWidth: true
            Layout.fillHeight: true

            ConversationList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                SplitView.preferredWidth: 300
                SplitView.minimumWidth: 200
                SplitView.maximumWidth: 500
            }

            MessageList {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
