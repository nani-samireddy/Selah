pragma ComponentBehavior: Bound
import QtQuick

Rectangle {
    id: tabs

    required property var options
    required property string currentValue

    signal chosen(string value)

    implicitWidth: items.implicitWidth + 8
    implicitHeight: 36
    radius: 8
    color: LibraryStyle.tone("#141414")
    border.color: LibraryStyle.divider

    Row {
        id: items

        x: 4
        y: 4
        spacing: 2

        Repeater {
            model: tabs.options

            LibraryButton {
                required property var modelData

                text: modelData.text
                selected: tabs.currentValue === modelData.value
                quiet: !selected
                implicitHeight: 28
                labelFontSize: 12

                onClicked: tabs.chosen(modelData.value)
            }
        }
    }
}
