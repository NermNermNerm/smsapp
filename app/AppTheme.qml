import QtQuick 2.15

QtObject {
    id: app2Palette

    property color white: "#ffffff"
    property color appOffWhite: "#f0f0f0"
    property color appLightGray: "#e0e0e0"
    property color appMediumGray: "#c0c0c0"
    property color appDarkGray: "#222222"

    // Core background colors
    property color windowBackground: appOffWhite
    property color titleBarBackground: appLightGray
    property color titleBarBorder: appMediumGray
    property color titleBarButtonTextColor: appDarkGray

    // Text colors
    property color text: "#000000"
    property color textDisabled: "#808080"

    // Status colors
    property color statusOk: "#00c853"
    property color statusWarn: "#ffca28"
    property color statusError: "#d32f2f"

    // Button colors
    property color buttonBackground: "#d0d0d0"
    property color buttonHover: "#e8e8e8"
    property color buttonText: "#000000"
}