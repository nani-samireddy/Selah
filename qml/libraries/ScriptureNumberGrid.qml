pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: grid

    required property var numbers
    property int selectedNumber: -1
    property string accessiblePrefix: ""

    signal chosen(int number)

    contentWidth: availableWidth
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    GridLayout {
        width: grid.availableWidth
        columns: 6
        columnSpacing: 5
        rowSpacing: 5

        Repeater {
            model: grid.numbers

            LibraryButton {
                required property int modelData

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                implicitHeight: 32
                implicitWidth: 28
                labelFontSize: 12
                text: modelData
                hint: grid.accessiblePrefix + " " + modelData
                selected: grid.selectedNumber === modelData

                onClicked: grid.chosen(modelData)
            }
        }
    }
}
