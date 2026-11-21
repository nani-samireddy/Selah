pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Owns panel placement; editing behavior lives in the child components.
Item {
    id: workspace

    required property var host

    function rendererProbe() {
        return viewport.rendererProbe();
    }

    function focusCanvas() {
        viewport.forceActiveFocus();
    }

    anchors.fill: parent

    Canvas {
        id: workspaceGrid

        property bool darkTheme: LibraryStyle.dark

        anchors.fill: parent

        onDarkThemeChanged: requestPaint()
        onPaint: {
            let ctx = getContext("2d");
            ctx.fillStyle = LibraryStyle.tone("#111111");
            ctx.fillRect(0, 0, width, height);
            ctx.fillStyle = LibraryStyle.tone("#2B2B2B");
            for (let x = 12; x < width; x += 20)
                for (let y = 12; y < height; y += 20) {
                    ctx.beginPath();
                    ctx.arc(x, y, 0.65, 0, Math.PI * 2);
                    ctx.fill();
                }
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    LayersPanel {
        id: layersPanel

        host: workspace.host
    }

    Column {
        x: 26
        y: layersPanel.y + layersPanel.height + 24
        width: 195
        spacing: 11

        StudioMicroLabel {
            text: "WORKSPACE"
        }

        Text {
            text: "Double-click text to edit.\nDrag a layer to move it.\nUse the corner to resize."
            color: LibraryStyle.tone("#777777")
            font.pixelSize: 11
            lineHeight: 1.7
        }

        Text {
            text: "⌘ Z  Undo    ·    ⌘ S  Save"
            color: LibraryStyle.tone("#686868")
            font.pixelSize: 10
        }
    }

    Item {
        id: centerArea

        anchors {
            left: layersPanel.right
            right: inspector.left
            top: parent.top
            bottom: parent.bottom
            leftMargin: 28
            rightMargin: 28
        }

        RowLayout {
            id: viewRow

            height: 34

            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                topMargin: 22
            }

            StudioButton {
                text: workspace.host.templateMode ? "Templates" : "Custom slides"
                iconName: "layers"
                hint: "Return to library"

                onClicked: workspace.host.browseLibrary(workspace.host.templateMode ? "templates" :
                                                                                      "custom")
            }

            Text {
                text: workspace.host.templateMode ? "Template layout" : "Custom presentation"
                color: LibraryStyle.muted
                font.pixelSize: 11
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                visible: !workspace.host.templateMode
                text: "Audience layout"
                color: LibraryStyle.tone("#959595")
                font.pixelSize: 11
            }
        }

        EditorCanvas {
            id: viewport

            host: workspace.host

            anchors {
                top: viewRow.bottom
                left: parent.left
                right: parent.right
                bottom: slideStrip.top
                topMargin: 12
                bottomMargin: 16
            }
        }

        SlideStrip {
            id: slideStrip

            host: workspace.host
            canvas: viewport

            anchors {
                left: parent.left
                right: parent.right
                bottom: notesBox.top
                bottomMargin: 16
            }
        }

        SpeakerNotes {
            id: notesBox

            host: workspace.host

            anchors {
                left: parent.left
                right: parent.right
                bottom: dock.top
                bottomMargin: 14
            }
        }

        EditorDock {
            id: dock

            host: workspace.host
        }
    }

    InspectorPanel {
        id: inspector

        host: workspace.host
    }

    Rectangle {
        width: toastText.implicitWidth + 32
        height: 36
        radius: 7
        color: LibraryStyle.tone("#303030")
        border.color: LibraryStyle.tone("#515151")
        visible: workspace.host.toast.length > 0
        z: 100

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 85
        }

        Text {
            id: toastText

            anchors.centerIn: parent
            text: workspace.host.toast
            color: LibraryStyle.tone("#F0F0F0")
            font.pixelSize: 12
        }
    }
}
