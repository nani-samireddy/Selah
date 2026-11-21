import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string labelFontFamily: LibraryStyle.fontFamily
    property int labelFontSize: 13
    property bool selected: false
    property bool accented: false
    property bool quiet: false
    property string hint: ""
    property string iconName: ""
    property bool feedbackEnabled: true
    readonly property bool active: checked || selected
    readonly property color fillColor: {
        if (!enabled)
            return quiet ? "transparent" : LibraryStyle.tone("#232323");
        if (accented) {
            if (down)
                return LibraryStyle.tone("#BDBDBD");
            return hovered ? LibraryStyle.tone("#FFFFFF") : LibraryStyle.tone("#E5E5E5");
        }
        if (active)
            return LibraryStyle.tone("#3A3A3A");
        if (down)
            return LibraryStyle.tone("#383838");
        if (hovered)
            return LibraryStyle.tone("#333333");
        return quiet ? "transparent" : LibraryStyle.tone("#292929");
    }
    readonly property color labelColor: {
        if (!enabled)
            return LibraryStyle.tone("#606060");
        if (accented)
            return LibraryStyle.tone("#171717");
        return active ? LibraryStyle.tone("#F1F1F1") : LibraryStyle.tone("#CBCBCB");
    }

    scale: down && enabled && !InteractionFeedback.reducedMotion ? 0.96 : 1
    padding: 0
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    implicitHeight: LibraryStyle.controlHeight
    implicitWidth: Math.max(28, label.implicitWidth + (iconName.length ? 23 : 0) + 16)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    ToolTip.visible: hovered && hint.length > 0
    ToolTip.text: hint
    ToolTip.delay: 500
    Accessible.name: hint.length ? hint : text

    Behavior on scale {
        NumberAnimation {
            duration: InteractionFeedback.reducedMotion ? 0 : 220
            easing.type: Easing.OutBack
            easing.overshoot: 0.6
        }
    }
    contentItem: Item {
        Row {
            anchors.centerIn: parent
            spacing: 7

            StudioIcon {
                visible: control.iconName.length > 0
                width: 14
                height: 14
                name: control.iconName
                tone: control.accented ? LibraryStyle.tone("#171717") : control.enabled
                                         ? LibraryStyle.tone("#B8B8B8") : LibraryStyle.tone(
                                               "#606060")
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: label

                renderType: Text.QtRendering
                text: control.text
                visible: text.length > 0
                color: control.labelColor
                font.family: control.labelFontFamily
                font.pixelSize: control.labelFontSize
                font.weight: control.accented ? Font.Medium : Font.Normal
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    background: Rectangle {
        radius: LibraryStyle.controlRadius
        color: control.fillColor
        border.width: control.visualFocus ? 1 : control.quiet || control.accented ? 0 : 1
        border.color: control.visualFocus ? LibraryStyle.tone("#AAAAAA") : LibraryStyle.tone(
                                                "#414141")

        Behavior on color {
            ColorAnimation {
                duration: InteractionFeedback.duration
            }
        }
    }

    Connections {
        function onClicked() {
            if (control.feedbackEnabled)
                InteractionFeedback.play(control.checkable || control.selected ? "select" :
                                                                                 "click");
        }

        target: control
    }
}
