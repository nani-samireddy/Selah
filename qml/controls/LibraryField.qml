import QtQuick
import QtQuick.Controls

TextField {
    id: field

    color: "#E0E0E0"
    placeholderTextColor: "#777777"
    font.pixelSize: 12
    selectByMouse: true
    padding: 10

    background: Rectangle {
        color: "#222222"
        border.color: field.activeFocus ? "#888888" : "#383838"
        radius: 6
    }
}
