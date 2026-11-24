import QtQuick

// Shared swatch + hex entry. Invalid partial input never reaches the document.
LibraryField {
    id: field

    property string value: "#FFFFFF"
    property string label: "Color"

    signal committed(string value)
    signal pickerRequested

    text: value.toUpperCase()
    leftPadding: 46
    Accessible.name: label + " hex value"

    validator: RegularExpressionValidator {
        regularExpression: /^#(?:[0-9a-fA-F]{3}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/
    }
    background: Rectangle {
        color: LibraryStyle.field
        radius: 7
        border.width: 1
        border.color: field.activeFocus ? LibraryStyle.focus : field.hovered ? LibraryStyle.tone(
                                                                                   "#505050") :
                                                                               LibraryStyle.border
    }

    onValueChanged: text = value.toUpperCase()
    onEditingFinished: {
        if (acceptableInput)
            committed(text.toUpperCase());
        else
            text = value.toUpperCase();
    }

    Rectangle {
        x: 36
        width: 1
        height: parent.height - 2
        anchors.verticalCenter: parent.verticalCenter
        color: LibraryStyle.border
    }

    StudioButton {
        objectName: "colorSwatchButton"
        x: 1
        y: 1
        width: 35
        height: parent.height - 2
        hint: "Choose " + field.label.toLowerCase()

        background: Item {
        }
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: 18
                height: 18
                radius: 4
                color: field.value
                border.width: 1
                border.color: LibraryStyle.dark ? "#55FFFFFF" : "#33000000"
            }
        }

        onClicked: field.pickerRequested()
    }
}
