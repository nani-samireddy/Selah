import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    font.pixelSize: 12
    palette.buttonText: "#DDDDDD"
    palette.text: "#DDDDDD"
    palette.highlight: "#444444"
    palette.highlightedText: "#FFFFFF"
    palette.base: "#242424"
    palette.window: "#242424"

    background: Rectangle {
        implicitHeight: 36
        color: "#262626"
        border.color: control.visualFocus ? "#888888" : "#404040"
        radius: 6
    }
}
