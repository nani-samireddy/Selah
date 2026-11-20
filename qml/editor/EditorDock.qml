pragma ComponentBehavior: Bound
import QtQuick

// Editor commands only; display controls live in the output and program panels.
StudioPanel {
    id: dock

    required property var host

    width: dockRow.implicitWidth + 14
    height: 46

    anchors {
        horizontalCenter: parent.horizontalCenter
        bottom: parent.bottom
        bottomMargin: 18
    }

    Row {
        id: dockRow

        anchors.centerIn: parent
        spacing: 4

        StudioButton {
            iconName: "mouse-pointer-2"
            quiet: true
            checked: !dock.host.handTool
            implicitWidth: 34
            hint: "Selection tool"

            onClicked: dock.host.handTool = false
        }

        StudioButton {
            iconName: "hand"
            quiet: true
            checked: dock.host.handTool
            implicitWidth: 34
            hint: "Pan tool — drag empty canvas"

            onClicked: dock.host.handTool = true
        }

        Rectangle {
            width: 1
            height: 20
            color: "#383838"
            anchors.verticalCenter: parent.verticalCenter
        }

        StudioButton {
            iconName: "type"
            quiet: true
            implicitWidth: 34
            hint: "Add text"

            onClicked: dock.host.doc.addText()
        }

        StudioButton {
            iconName: "image"
            quiet: true
            implicitWidth: 34
            hint: "Open media library"
            enabled: !dock.host.doc.importing

            onClicked: dock.host.showLibrary()
        }

        StudioButton {
            text: dock.host.videosPlaying ? "Pause" : "Play"
            quiet: true
            hint: "Play or pause videos in the editor (muted)"

            onClicked: dock.host.videosPlaying = !dock.host.videosPlaying
        }

        Rectangle {
            width: 1
            height: 20
            color: "#383838"
            anchors.verticalCenter: parent.verticalCenter
        }

        StudioButton {
            iconName: "undo-2"
            quiet: true
            implicitWidth: 32
            hint: "Undo"
            enabled: dock.host.doc.canUndo

            onClicked: dock.host.doc.undo()
        }

        StudioButton {
            iconName: "redo-2"
            quiet: true
            implicitWidth: 32
            hint: "Redo"
            enabled: dock.host.doc.canRedo

            onClicked: dock.host.doc.redo()
        }

        Rectangle {
            width: 1
            height: 20
            color: "#383838"
            anchors.verticalCenter: parent.verticalCenter
        }

        StudioButton {
            iconName: "minus"
            quiet: true
            implicitWidth: 27
            hint: "Zoom out"

            onClicked: dock.host.zoom = Math.max(0.4, dock.host.zoom - 0.1)
        }

        StudioButton {
            text: Math.round(dock.host.zoom * 100) + "%"
            quiet: true
            implicitWidth: 47
            hint: "Fit canvas"

            onClicked: {
                dock.host.zoom = 1;
                dock.host.panX = 0;
                dock.host.panY = 0;
            }
        }

        StudioButton {
            iconName: "plus"
            quiet: true
            implicitWidth: 27
            hint: "Zoom in"

            onClicked: dock.host.zoom = Math.min(2.5, dock.host.zoom + 0.1)
        }
    }
}
