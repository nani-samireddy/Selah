import QtQuick
import QtQuick.Controls

LibraryButton {
    id: control

    objectName: "feedbackSettings"
    implicitWidth: LibraryStyle.controlHeight
    implicitHeight: LibraryStyle.controlHeight
    quiet: true
    feedbackEnabled: false
    hint: (InteractionFeedback.soundEnabled ? "Mute interface sounds" : "Enable interface sounds")
          + " · Right-click for motion settings"

    contentItem: Item {
        StudioIcon {
            anchors.centerIn: parent
            width: 17
            height: 17
            tone: LibraryStyle.muted
            source: "data:image/svg+xml," + encodeURIComponent(
                        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="#cccccc" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"><path d="m11 5-6 4H2v6h3l6 4Z"/>'
                        + (InteractionFeedback.soundEnabled
                           ? '<path d="M15 8a6 6 0 0 1 0 8m3-11a10 10 0 0 1 0 14"/>' :
                             '<path d="m16 9 6 6m0-6-6 6"/>') + '</svg>')
        }
    }

    onClicked: {
        InteractionFeedback.setSounds(!InteractionFeedback.soundEnabled);
        InteractionFeedback.play("select");
    }
    onPressAndHold: options.open()

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton

        onClicked: options.open()
    }

    Popup {
        id: options

        x: Math.min(0, control.width - width)
        y: control.height + 4
        padding: 6

        background: Rectangle {
            radius: 8
            color: LibraryStyle.panel
            border.color: LibraryStyle.border
        }
        contentItem: Column {
            spacing: 4

            LibraryButton {
                text: "Interface sounds"
                width: 180
                checkable: true
                checked: InteractionFeedback.soundEnabled

                onClicked: InteractionFeedback.setSounds(checked)
            }

            LibraryButton {
                objectName: "reducedMotionPreference"
                text: "Reduced motion"
                width: 180
                checkable: true
                checked: InteractionFeedback.reducedMotion

                onClicked: InteractionFeedback.setReducedMotion(checked)
            }
        }
    }
}
