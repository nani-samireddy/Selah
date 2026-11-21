import QtQuick
import QtQuick.Controls

TextArea {
    id: field

    renderType: Text.QtRendering
    color: LibraryStyle.text
    placeholderTextColor: LibraryStyle.tone("#737373")
    selectionColor: LibraryStyle.tone("#484848")
    selectedTextColor: LibraryStyle.tone("#FFFFFF")
    font.family: LibraryStyle.fontFamily
    font.weight: Font.Normal
    font.pixelSize: 16
    padding: 12
    selectByMouse: true
    wrapMode: TextEdit.Wrap

    background: Rectangle {
        color: LibraryStyle.field
        radius: LibraryStyle.controlRadius
        border.color: field.activeFocus ? LibraryStyle.focus : LibraryStyle.border
    }
}
