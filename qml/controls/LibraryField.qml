import QtQuick
import QtQuick.Controls

TextField {
    id: field

    property string leadingIcon: ""

    renderType: Text.QtRendering
    implicitHeight: LibraryStyle.controlHeight
    implicitWidth: 140
    color: enabled ? LibraryStyle.text : LibraryStyle.tone("#666666")
    placeholderTextColor: LibraryStyle.tone("#777777")
    selectionColor: LibraryStyle.tone("#484848")
    selectedTextColor: LibraryStyle.tone("#FFFFFF")
    font.family: LibraryStyle.fontFamily
    font.weight: Font.Normal
    font.pixelSize: 12
    selectByMouse: true
    hoverEnabled: true
    leftPadding: leadingIcon.length ? 30 : 10
    rightPadding: 10
    topPadding: 4
    bottomPadding: 4

    background: Rectangle {
        color: LibraryStyle.field
        border.color: field.activeFocus ? LibraryStyle.focus : field.hovered ? LibraryStyle.tone(
                                                                                   "#484848") :
                                                                               LibraryStyle.border
        radius: LibraryStyle.controlRadius
        opacity: field.enabled ? 1 : 0.5
    }

    StudioIcon {
        visible: field.leadingIcon.length > 0
        name: field.leadingIcon
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 13
        height: 13
        tone: LibraryStyle.tone("#818181")
    }
}
