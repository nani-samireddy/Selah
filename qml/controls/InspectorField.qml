import QtQuick
import QtQuick.Controls

Rectangle {
    id: field

    property string caption: ""
    property string value: ""
    property bool numeric: true
    readonly property string currentText: input.text

    signal committed(string value)

    implicitHeight: LibraryStyle.controlHeight
    color: LibraryStyle.tone("#252525")
    border.color: input.activeFocus ? LibraryStyle.tone("#888888") : LibraryStyle.tone("#383838")
    radius: 5

    Text {
        renderType: Text.QtRendering
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        text: field.caption
        color: LibraryStyle.tone("#818181")
        font.pixelSize: 11
    }

    TextField {
        id: input

        renderType: Text.QtRendering
        text: field.value
        color: LibraryStyle.tone("#DDDDDD")
        font.weight: Font.Normal
        font.pixelSize: 12
        selectByMouse: true
        horizontalAlignment: field.caption.length ? TextInput.AlignRight : TextInput.AlignLeft
        padding: 5
        validator: field.numeric ? numberValidator : null
        Accessible.name: field.caption

        background: Item {
        }

        onEditingFinished: field.committed(text)

        anchors {
            left: parent.left
            leftMargin: field.caption.length ? 30 : 6
            right: parent.right
            rightMargin: 4
            verticalCenter: parent.verticalCenter
        }

        DoubleValidator {
            id: numberValidator

            bottom: -360
            top: 10000
            decimals: 1
            locale: "en_US"
        }
    }
}
