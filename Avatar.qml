import QtQuick 2.15
import Sms 1.0

Item {
    id: root
    property int size: 40

    // Public-facing property
    property string participants: ""   // mirrored into model.participants

    width: size
    height: size

    AvatarModel {
        id: model
        participants: root.participants
    }

    // One participant → single avatar
    SingleAvatar {
        id: single
        visible: model.participant2.length === 0
        participant: model.participant1
        size: root.size
        anchors.centerIn: parent
    }

    // Two participants → side-by-side
    Item {
        id: two
        visible: model.participant2.length > 0 && model.participant3.length === 0
        anchors.centerIn: parent
        width: root.size
        height: root.size

        SingleAvatar {
            participant: model.participant1
            size: root.size * 0.6
            anchors.left: parent.left
            anchors.top: parent.top
        }

        SingleAvatar {
            participant: model.participant2
            size: root.size * 0.6
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }

    // Three participants → triangular layout
    Item {
        id: three
        visible: model.participant3.length > 0 && model.participant4.length === 0
        anchors.centerIn: parent
        width: root.size
        height: root.size

        SingleAvatar {
            participant: model.participant1
            size: root.size * 0.55
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        SingleAvatar {
            participant: model.participant2
            size: root.size * 0.55
            anchors.bottom: parent.bottom
            anchors.left: parent.left
        }

        SingleAvatar {
            participant: model.participant3
            size: root.size * 0.55
            anchors.bottom: parent.bottom
            anchors.right: parent.right
        }
    }

    // Four participants → 2×2 grid
    Item {
        id: four
        visible: model.participant4.length > 0
        anchors.centerIn: parent
        width: root.size
        height: root.size

        SingleAvatar {
            participant: model.participant1
            size: root.size * 0.50
            anchors.top: parent.top
            anchors.left: parent.left
        }

        SingleAvatar {
            participant: model.participant2
            size: root.size * 0.50
            anchors.top: parent.top
            anchors.right: parent.right
        }

        SingleAvatar {
            participant: model.participant3
            size: root.size * 0.50
            anchors.bottom: parent.bottom
            anchors.left: parent.left
        }

        SingleAvatar {
            participant: model.participant4
            size: root.size * 0.50
            anchors.bottom: parent.bottom
            anchors.right: parent.right
        }
    }
}
