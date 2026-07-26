import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    // --- Public Inputs ---
    property int charge: 75          // Value from 0 to 100
    property bool isCharging: false   // Charging status flag
    property bool showText: true      // Toggle text readout on/off
    property string textPosition: "right" // "right" or "left"

    // --- Styling Properties ---
    property color batteryColor: {
        if (root.charge <= 20) return "#E74C3C"      // Warning Red
        if (root.charge <= 40) return "#F39C12"      // Caution Orange/Yellow
        return "#2ECC71"                             // Healthy Green
    }
    property color outlineColor: "#2C3E50"
    property color chargingBoltColor: "#F1C40F"      // High visibility Accent Yellow
    property color textColor: root.outlineColor

    // --- Sizing & Scaling Logic ---
    implicitHeight: 24
    height: 24

    readonly property real iconWidth: Math.round(root.height * 0.55)
    readonly property real calculatedWidth: {
        var w = root.iconWidth
        if (root.isCharging && root.height <= 20) w += 10 // Extra room for side-bolt at tiny sizes
        if (root.showText) w += 6 + percentLabel.implicitWidth
        return w
    }
    implicitWidth: root.calculatedWidth
    width: implicitWidth

    readonly property real clampedCharge: Math.max(0, Math.min(100, root.charge))
    readonly property real capWidth: Math.max(3, Math.round(root.iconWidth * 0.35))
    readonly property real capHeight: Math.max(2, Math.round(root.height * 0.08))

    HoverHandler {
        id: hoverHandler
    }

    ToolTip {
        visible: hoverHandler.hovered
        delay: 400
        timeout: 4000
        text: deviceStatus.isCharging
                      ? qsTr( "%1% - Charging").arg(deviceStatus.batteryCharge.toString())
                      : qsTr( "%1%").arg(deviceStatus.batteryCharge.toString())
    }

    RowLayout {
        anchors.fill: parent
        spacing: 4
        layoutDirection: root.textPosition === "left" ? Qt.RightToLeft : Qt.LeftToRight

        // --- Side-by-Side Bolt Indicator for Small Heights (≤ 20px) ---
        Canvas {
            id: sideBoltCanvas
            visible: root.isCharging && root.height <= 20
            Layout.preferredWidth: 8
            Layout.preferredHeight: root.height - 2
            Layout.alignment: Qt.AlignVCenter

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();
                ctx.fillStyle = root.chargingBoltColor;
                ctx.beginPath();
                // Sharp vector lightning path
                var w = width;
                var h = height;
                ctx.moveTo(w * 0.55, 0);
                ctx.lineTo(0, h * 0.55);
                ctx.lineTo(w * 0.45, h * 0.55);
                ctx.lineTo(w * 0.35, h);
                ctx.lineTo(w, h * 0.42);
                ctx.lineTo(w * 0.55, h * 0.42);
                ctx.closePath();
                ctx.fill();
            }

            Connections {
                target: root
                function onChargingBoltColorChanged() { sideBoltCanvas.requestPaint(); }
            }
        }

        // --- Vertical Battery Shell ---
        Item {
            Layout.preferredWidth: root.iconWidth
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter

            // Top Positive Terminal (+ Cap)
            Rectangle {
                id: terminalCap
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.capWidth
                height: root.capHeight
                radius: 1
                color: root.outlineColor
            }

            // Main Body Box
            Rectangle {
                id: body
                anchors.top: terminalCap.bottom
                anchors.topMargin: 1
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                radius: Math.max(1, Math.round(root.height * 0.08))
                color: "transparent"
                border.color: root.outlineColor
                border.width: root.height <= 20 ? 1 : 2

                // Inner Meter Area
                Item {
                    anchors.fill: parent
                    anchors.margins: body.border.width + 0.5

                    // Liquid Fill Meter (Fills Bottom to Top)
                    Rectangle {
                        id: fillBar
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height * (root.clampedCharge / 100)
                        radius: Math.max(0, body.radius - 1)
                        color: root.batteryColor

                        Behavior on height {
                            NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
                        }
                        Behavior on color {
                            ColorAnimation { duration: 300 }
                        }
                    }

                    // Vector Overlay Bolt for Larger Sizes (> 20px)
                    Canvas {
                        id: overlayBoltCanvas
                        anchors.centerIn: parent
                        width: Math.max(6, parent.width * 0.8)
                        height: Math.max(8, parent.height * 0.75)
                        visible: root.isCharging && root.height > 20

                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.reset();

                            // High contrast color: if fill height covers middle, draw pure white with dark border
                            ctx.fillStyle = "#FFFFFF";
                            ctx.strokeStyle = "#1A252C";
                            ctx.lineWidth = 1;

                            ctx.beginPath();
                            var w = width;
                            var h = height;
                            ctx.moveTo(w * 0.6, 0);
                            ctx.lineTo(w * 0.1, h * 0.55);
                            ctx.lineTo(w * 0.5, h * 0.55);
                            ctx.lineTo(w * 0.3, h);
                            ctx.lineTo(w * 0.9, h * 0.42);
                            ctx.lineTo(w * 0.5, h * 0.42);
                            ctx.closePath();
                            ctx.fill();
                            ctx.stroke();
                        }

                        // Pulse Animation -- a little too much sauce, I think.
                        // SequentialAnimation on opacity {
                        //     running: overlayBoltCanvas.visible
                        //     loops: Animation.Infinite
                        //     PropertyAnimation { to: 0.3; duration: 750 }
                        //     PropertyAnimation { to: 1.0; duration: 750 }
                        // }

                        Connections {
                            target: root
                            function onChargeChanged() { overlayBoltCanvas.requestPaint(); }
                        }
                    }
                }
            }
        }

        // --- Percentage Readout Label ---
        Label {
            id: percentLabel
            visible: root.showText
            text: root.clampedCharge + "%"
            font.pixelSize: Math.max(9, Math.round(root.height * 0.55))
            font.weight: Font.Medium
            color: root.textColor
            Layout.alignment: Qt.AlignVCenter
        }
    }
}