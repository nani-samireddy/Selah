pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Item {
    id: heading

    property string label: ""
    property bool expanded: true

    signal resetRequested

    Layout.fillWidth: true
    implicitHeight: 27

    StudioMicroLabel {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: heading.label
    }

    Row {
        anchors.right: parent.right
        spacing: 1

        StudioButton {
            iconName: "rotate-ccw"
            quiet: true
            implicitWidth: 25
            implicitHeight: 25
            hint: "Reset section"

            onClicked: heading.resetRequested()
        }

        StudioButton {
            iconName: heading.expanded ? "chevron-up" : "chevron-down"
            quiet: true
            implicitWidth: 25
            implicitHeight: 25
            hint: "Collapse or expand section"

            onClicked: heading.expanded = !heading.expanded
        }
    }
}
