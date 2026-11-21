pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

// Edits private draft notes; audience rendering never displays this control.
Rectangle {
    id: notesBox

    required property var host
    property bool expanded: false

    height: expanded ? 69 : 32
    color: LibraryStyle.tone("#181818")
    border.color: LibraryStyle.tone("#2D2D2D")
    radius: 7

    Text {
        x: 13
        y: 10
        text: "SPEAKER NOTES"
        color: LibraryStyle.tone("#747474")
        font.pixelSize: 9
        font.letterSpacing: 1
    }

    StudioButton {
        anchors.right: parent.right
        anchors.rightMargin: 6
        iconName: notesBox.expanded ? "chevron-up" : "chevron-down"
        implicitHeight: 30
        quiet: true
        hint: "Expand speaker notes"

        onClicked: notesBox.expanded = !notesBox.expanded
    }

    TextField {
        visible: notesBox.expanded
        text: notesBox.host.doc.notes
        placeholderText: "Add a note for the speaker…"
        placeholderTextColor: LibraryStyle.tone("#626262")
        font.pixelSize: 11
        color: LibraryStyle.tone("#BBBBBB")
        selectByMouse: true

        background: Item {
        }

        onEditingFinished: notesBox.host.doc.notes = text

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 26
            bottom: parent.bottom
            leftMargin: 8
            rightMargin: 8
        }
    }
}
