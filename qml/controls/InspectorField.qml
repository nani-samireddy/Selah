import QtQuick
import QtQuick.Controls

Rectangle {
    id: field

    property string caption: ""
    property bool captionInteractive: false
    property string value: ""
    property bool numeric: true
    property real stepSize: 1
    property real minimumValue: -360
    property real maximumValue: 10000
    property int decimals: 1
    readonly property string currentText: input.text

    signal committed(string value)
    signal captionClicked

    function step(direction, modifiers) {
        const typed = Number(input.text);
        const current = input.text.trim().length && Number.isFinite(typed) ? typed : Number(value);
        if (!Number.isFinite(current))
            return;
        const multiplier = modifiers & Qt.ShiftModifier ? 10 : 1;
        const amount = Math.max(minimumValue, Math.min(maximumValue, current + direction * stepSize
                                                       * multiplier));
        input.text = String(Number(amount.toFixed(decimals)));
        committed(input.text);
    }

    implicitHeight: LibraryStyle.controlHeight
    color: LibraryStyle.tone("#252525")
    border.color: input.activeFocus ? LibraryStyle.tone("#888888") : LibraryStyle.tone("#383838")
    radius: 5

    onValueChanged: input.text = value

    Text {
        visible: !field.captionInteractive
        renderType: Text.QtRendering
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        text: field.caption
        color: LibraryStyle.tone("#818181")
        font.pixelSize: 11
    }

    ToolButton {
        objectName: field.captionInteractive ? "inspectorUnitButton" : ""
        visible: field.captionInteractive
        x: 4
        width: 40
        height: field.height - 4
        anchors.verticalCenter: parent.verticalCenter
        Accessible.name: "Change " + field.caption + " unit"
        ToolTip.visible: hovered
        ToolTip.text: "Change unit"

        background: Item {
        }
        contentItem: Row {
            spacing: 3

            Text {
                text: field.caption
                color: LibraryStyle.muted
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            StudioIcon {
                name: "chevron-down"
                width: 8
                height: 8
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        onClicked: field.captionClicked()
    }

    TextField {
        id: input

        objectName: "inspectorNumberInput"
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

        Keys.onUpPressed: event => {
            event.accepted = field.numeric;
            if (field.numeric)
                field.step(1, event.modifiers);
        }
        Keys.onDownPressed: event => {
            event.accepted = field.numeric;
            if (field.numeric)
                field.step(-1, event.modifiers);
        }
        onEditingFinished: field.committed(text)

        anchors {
            left: parent.left
            leftMargin: field.captionInteractive ? 48 : field.caption.length ? 30 : 6
            right: parent.right
            rightMargin: 4
            verticalCenter: parent.verticalCenter
        }

        DoubleValidator {
            id: numberValidator

            bottom: field.minimumValue
            top: field.maximumValue
            decimals: field.decimals
            locale: "en_US"
        }
    }
}
