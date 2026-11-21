pragma Singleton
import QtQuick
import QtCore

QtObject {
    property var platformHints: Qt.styleHints
    property Settings preferences: Settings {
        id: appearancePreferences

        property string theme: "dark"

        category: "Appearance"
    }
    readonly property string appearance: appearancePreferences.theme
    readonly property bool dark: appearance === "dark" || (appearance === "system"
                                                           && platformHints.colorScheme === Qt.Dark)
    readonly property string fontFamily: "Inter"
    readonly property color background: tone("#111111")
    readonly property color panel: tone("#171717")
    readonly property color field: tone("#202020")
    readonly property color hover: tone("#282828")
    readonly property color border: tone("#343434")
    readonly property color divider: tone("#2B2B2B")
    readonly property color text: tone("#E6E6E6")
    readonly property color muted: tone("#969696")
    readonly property color focus: tone("#858585")
    readonly property int controlHeight: 28
    readonly property int controlRadius: 8
    readonly property int pagePadding: 24
    readonly property int fieldGap: 6
    readonly property int sectionGap: 16

    function setAppearance(value) {
        if (!["system", "light", "dark"].includes(value))
            return;
        appearancePreferences.theme = value;
        platformHints.colorScheme = value === "system" ? Qt.Unknown : value === "dark" ? Qt.Dark :
                                                                                         Qt.Light;
    }

    // Preserve the established neutral hierarchy while adapting older controls.
    function tone(value) {
        if (dark)
            return value;
        const level = parseInt(value.slice(1, 3), 16);
        if (level <= 17)
            return "#FAFAFA";
        if (level <= 25)
            return "#FFFFFF";
        if (level <= 35)
            return "#F5F5F5";
        if (level <= 48)
            return "#EDEDED";
        if (level <= 68)
            return "#D6D6D6";
        if (level <= 90)
            return "#BDBDBD";
        if (level <= 118)
            return "#707070";
        if (level <= 160)
            return "#5E5E5E";
        if (level <= 200)
            return "#424242";
        return "#1D1D1D";
    }

    Component.onCompleted: setAppearance(appearance)
}
