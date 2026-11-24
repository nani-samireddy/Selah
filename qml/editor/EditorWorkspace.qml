pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Owns panel placement; editing behavior lives in the child components.
Item {
    id: workspace

    required property var host
    property bool slidesPanelVisible: true

    function rendererProbe() {
        return viewport.rendererProbe();
    }

    function focusCanvas() {
        viewport.forceActiveFocus();
    }

    anchors.fill: parent

    Item {
        id: canvasScene

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

        EditorCanvas {
            id: viewport

            objectName: "editorCanvasViewport"
            host: workspace.host
            anchors.fill: parent
        }
    }

    EditorBackdrop {
        id: backdrop

        anchors.fill: parent
        scene: canvasScene
        active: workspace.visible
    }

    LayersPanel {
        id: layersPanel

        backdropSource: backdrop.available ? backdrop.texture : null
        backdropRect: Qt.rect(layersPanel.x, layersPanel.y, layersPanel.width, layersPanel.height)
        objectName: "editorLayersPanel"
        z: 20
        host: workspace.host
    }

    // These overlays remain fixed while the workspace underneath pans and zooms.
    Item {
        id: bottomControls

        z: 20

        anchors {
            left: layersPanel.right
            right: inspector.left
            top: parent.top
            bottom: parent.bottom
            leftMargin: 18
            rightMargin: 18
        }

        Item {
            id: trayFrame

            objectName: "editorFloatingControls"
            visible: workspace.host.templateMode || workspace.slidesPanelVisible
                     || panelMotion.animating
            height: slideStrip.height + notesBox.height + (!workspace.host.templateMode ? 12 : 0)
                    + 20

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                bottomMargin: 18
            }

            StudioPanel {
                id: contentTools

                anchors.fill: parent
                backdropSource: backdrop.available ? backdrop.texture : null
                backdropRect: Qt.rect(bottomControls.x + trayFrame.x, bottomControls.y + trayFrame.y,
                                      trayFrame.width, trayFrame.height)

                SlideStrip {
                    id: slideStrip

                    host: workspace.host
                    canvas: viewport

                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        margins: 10
                    }
                }

                SpeakerNotes {
                    id: notesBox

                    host: workspace.host

                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        margins: 10
                    }
                }
            }
        }

        PanelGenie {
            id: panelMotion

            objectName: "slidesPanelMotion"
            panel: trayFrame
            expanded: workspace.host.templateMode || workspace.slidesPanelVisible
            motionEnabled: !workspace.host.templateMode && workspace.visible
            targetPoint: Qt.point(dock.x + dock.slidesButtonCenter.x - trayFrame.x, dock.y
                                  + dock.slidesButtonCenter.y - trayFrame.y)
        }

        StudioPanel {
            id: compactSlides

            objectName: "compactEditorSlides"
            visible: !workspace.host.templateMode && !workspace.slidesPanelVisible &&
                     !panelMotion.animating
            width: Math.max(70, Math.min(260, bottomControls.width - dock.width - 10))
            height: dock.height
            x: dock.x + dock.width + 10
            y: dock.y
            z: 2
            backdropSource: backdrop.available ? backdrop.texture : null
            backdropRect: Qt.rect(bottomControls.x + x, bottomControls.y + y, width, height)

            SlideStrip {
                objectName: "compactEditorSlideStrip"
                host: workspace.host
                canvas: viewport
                compact: true
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 6
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        EditorDock {
            id: dock

            backdropSource: backdrop.available ? backdrop.texture : null
            backdropRect: Qt.rect(bottomControls.x + dock.x, bottomControls.y + dock.y, dock.width,
                                  dock.height)
            objectName: "editorFloatingDock"
            z: 2
            anchors.bottomMargin: 18 + (trayFrame.height + 12) * (1 - panelMotion.progress)
            anchors.horizontalCenterOffset: workspace.host.templateMode ? 0 : -(compactSlides.width
                                                                                + 10) / 2
                                                                          * panelMotion.progress
            host: workspace.host
            slidesPanelVisible: workspace.slidesPanelVisible

            onSlidesPanelToggleRequested: {
                notesBox.flush();
                workspace.slidesPanelVisible = !workspace.slidesPanelVisible;
            }
        }
    }

    InspectorPanel {
        id: inspector

        backdropSource: backdrop.available ? backdrop.texture : null
        backdropRect: Qt.rect(inspector.x, inspector.y, inspector.width, inspector.height)
        objectName: "editorInspectorPanel"
        z: 20
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
