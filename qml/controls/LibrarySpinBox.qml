pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

SpinBox {
    id: control

    implicitHeight: LibraryStyle.controlHeight
    implicitWidth: 96
    editable: true
    leftPadding: 28
    rightPadding: 28
    font.family: LibraryStyle.fontFamily
    font.weight: Font.Normal
    font.pixelSize: 12

    validator: IntValidator {
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }
    background: Rectangle {
        radius: LibraryStyle.controlRadius
        color: LibraryStyle.field
        border.color: control.activeFocus ? LibraryStyle.focus : LibraryStyle.border
    }
    contentItem: TextInput {
        renderType: Text.QtRendering
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: LibraryStyle.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        selectByMouse: true
        selectionColor: LibraryStyle.tone("#484848")
        selectedTextColor: LibraryStyle.tone("#FFFFFF")
    }
    down.indicator: Rectangle {
        x: 1
        y: 1
        width: 26
        height: control.height - 2
        radius: 5
        color: control.down.pressed ? LibraryStyle.tone("#383838") : control.down.hovered
                                      ? LibraryStyle.tone("#2B2B2B") : "transparent"

        StudioIcon {
            anchors.centerIn: parent
            name: "minus"
            width: 12
            height: 12
            tone: control.value > control.from ? LibraryStyle.tone("#BBBBBB") : LibraryStyle.tone(
                                                     "#555555")

        }
    }
    up.indicator: Rectangle {
        x: control.width - width - 1
        y: 1
        width: 26
        height: control.height - 2
        radius: 5
        color: control.up.pressed ? LibraryStyle.tone("#383838") : control.up.hovered ? LibraryStyle.tone(
                                                                                            "#2B2B2B") :
                                                                                        "transparent"

        StudioIcon {
            anchors.centerIn: parent
            name: "plus"
            width: 12
            height: 12
            tone: control.value < control.to ? LibraryStyle.tone("#BBBBBB") : LibraryStyle.tone(
                                                   "#555555")

        }
    }
}
