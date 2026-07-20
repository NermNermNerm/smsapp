import QtQuick 2.15
import QtQuick.Controls 2.15
import Sms 1.0

Item {
    id: root
    property int size: 40
    property string participant: ""

    width: size
    height: size

    SingleAvatarModel {
        id: model
        phoneNumber: root.participant
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: model.color
    }

    Text {
        anchors.centerIn: parent
        text: model.initials
        visible: model.initials.length > 0
        font.pixelSize: size * 0.55
        font.bold: true
        color: "white"
    }

    Canvas {
        id: silhouette
        anchors.fill: parent
        visible: model.initials.length == 0
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.fillStyle = "white";

            // Head: circle
            var headRadius = size * 0.19;
            var headCenterX = size * 0.5;
            var headCenterY = size * 0.33;

            ctx.beginPath();
            ctx.arc(headCenterX, headCenterY, headRadius, 0, Math.PI * 2);
            ctx.fill();

            // Torso: football shape
            var torsoTopY = size * 0.48;
            var torsoBottomY = size * 1.0;
            var torsoWidth = size * 0.55;

            ctx.beginPath();
            ctx.moveTo(headCenterX - torsoWidth / 2, (torsoTopY + torsoBottomY)/2);
            ctx.quadraticCurveTo(
                headCenterX, torsoTopY,
                headCenterX + torsoWidth / 2, (torsoTopY + torsoBottomY)/2
            );
            ctx.quadraticCurveTo(
                headCenterX, torsoBottomY,
                headCenterX - torsoWidth / 2, (torsoTopY + torsoBottomY)/2
            );
            ctx.fill();
        }
    }
}
