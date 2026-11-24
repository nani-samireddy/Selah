pragma ComponentBehavior: Bound
import QtQuick

// Owns pan/zoom and transient edit gestures; commits completed edits to the document.
Item {
    id: viewport

    required property var host
    property real fitScale: Math.max(0.05, Math.min((width - 24) / 1920, (height - 64) / 1080))

    function rendererProbe() {
        return editorRenderer.probe();
    }

    // Preserve the canvas point beneath the cursor instead of zooming about the center.
    function zoomAt(factor, point) {
        if (!isFinite(factor) || factor <= 0)
            return;
        const before = host.zoom;
        const after = Math.max(0.4, Math.min(2.5, before * factor));
        const ratio = after / before;
        host.panX = point.x - width / 2 - (point.x - width / 2 - host.panX) * ratio;
        host.panY = point.y - height / 2 - (point.y - height / 2 - host.panY) * ratio;
        host.zoom = after;
    }

    clip: true

    WheelHandler {
        objectName: "canvasWheelHandler"
        target: null
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

        onWheel: event => {
            const pixels = event.pixelDelta;
            const modified = event.modifiers & (Qt.ControlModifier | Qt.MetaModifier);
            if (!modified && (pixels.x !== 0 || pixels.y !== 0)) {
                viewport.host.panX += pixels.x;
                viewport.host.panY += pixels.y;
            } else {
                const delta = pixels.y || event.angleDelta.y;
                if (delta !== 0)
                    viewport.zoomAt(Math.exp(delta * (pixels.y ? 0.008 : 0.001)), Qt.point(event.x,
                                                                                           event.y));
            }
            event.accepted = true;
        }
    }

    PinchHandler {
        objectName: "canvasPinchHandler"
        target: null
        acceptedDevices: PointerDevice.TouchPad | PointerDevice.TouchScreen
        rotationAxis.enabled: false
        // Native trackpads report pan through wheel pixels, not pinch translation.
        xAxis.enabled: false
        yAxis.enabled: false

        onScaleChanged: delta => viewport.zoomAt(delta, centroid.position)
    }

    MouseArea {
        anchors.fill: parent

        onClicked: viewport.host.selectCanvasSettings()
    }

    // Above element hit targets: panning must also work over text, images and handles.
    MouseArea {
        property point origin
        property point panOrigin

        objectName: "canvasPanSurface"
        z: 20000
        anchors.fill: parent
        acceptedButtons: viewport.host.handTool ? Qt.LeftButton | Qt.MiddleButton : Qt.MiddleButton
        cursorShape: pressed ? Qt.ClosedHandCursor : viewport.host.handTool ? Qt.OpenHandCursor :
                                                                              Qt.ArrowCursor

        onPressed: mouse => {
            viewport.forceActiveFocus();
            origin = Qt.point(mouse.x, mouse.y);
            panOrigin = Qt.point(viewport.host.panX, viewport.host.panY);
        }
        onPositionChanged: mouse => {
            if (pressed) {
                viewport.host.panX = panOrigin.x + mouse.x - origin.x;
                viewport.host.panY = panOrigin.y + mouse.y - origin.y;
            }
        }
    }

    Item {
        id: artboardFrame

        width: 1920 * viewport.fitScale * viewport.host.zoom
        height: 1080 * viewport.fitScale * viewport.host.zoom
        x: (viewport.width - width) / 2 + viewport.host.panX
        y: (viewport.height - height) / 2 + viewport.host.panY

        Text {
            text: String(viewport.host.doc.currentSlide + 1).padStart(2, "0") + "  /  "
                  + viewport.host.doc.slides[viewport.host.doc.currentSlide].name
            color: LibraryStyle.tone("#949494")
            font.pixelSize: 10

            anchors {
                left: parent.left
                bottom: parent.top
                bottomMargin: 12
            }
        }

        Text {
            text: "1920 × 1080"
            color: LibraryStyle.tone("#686868")
            font.pixelSize: 10

            anchors {
                right: parent.right
                bottom: parent.top
                bottomMargin: 12
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            color: "transparent"
            border.color: "#48404D"
        }

        Item {
            id: artboard

            objectName: "editorArtboard"
            width: 1920
            height: 1080
            scale: viewport.fitScale * viewport.host.zoom
            transformOrigin: Item.TopLeft

            SlideRenderer {
                id: editorRenderer

                enabled: false
                showTransparencyGrid: true
                objectName: "editorSlideRenderer"
                elements: viewport.host.doc.elements
                backgroundFill: viewport.host.doc.backgroundFill
                backgroundColor: viewport.host.doc.backgroundColor
                ambientOpacity: viewport.host.doc.ambientOpacity
                playing: viewport.host.videosPlaying
                audible: false
                suppressedId: viewport.host.editingId

                onMediaProblem: message => viewport.host.notify(message)
            }

            MouseArea {
                anchors.fill: parent
                enabled: !viewport.host.handTool

                onClicked: {
                    viewport.host.selectCanvasSettings();
                }
            }

            RichTextEditor {
                id: richEditor

                host: viewport.host
                z: 10000
            }

            Repeater {
                model: viewport.host.doc.elements

                delegate: Item {
                    id: objectBlock

                    required property var modelData
                    required property int index
                    readonly property bool editing: viewport.host.editingId === modelData.uid
                    property bool selected: viewport.host.doc.selected === index

                    function previewGeometry() {
                        editorRenderer.gestureGeometry = {
                            x: x,
                            y: y,
                            width: width,
                            height: height
                        };
                        editorRenderer.gestureId = modelData.uid;
                    }

                    x: modelData.x
                    y: modelData.y
                    width: modelData.width
                    height: modelData.height
                    rotation: modelData.rotation
                    opacity: modelData.visible ? modelData.opacity : 0
                    visible: modelData.visible

                    MouseArea {
                        id: moveMouse

                        anchors.fill: parent
                        enabled: !objectBlock.editing && !viewport.host.handTool
                        cursorShape: objectBlock.modelData.locked ? Qt.ForbiddenCursor : pressed
                                                                    ? Qt.ClosedHandCursor :
                                                                      Qt.OpenHandCursor
                        drag.target: objectBlock.modelData.locked ? null : objectBlock
                        drag.minimumX: 0
                        drag.maximumX: 1920 - objectBlock.width
                        drag.minimumY: 0
                        drag.maximumY: 1080 - objectBlock.height

                        onPressed: {
                            viewport.forceActiveFocus();
                            viewport.host.doc.selected = objectBlock.index;
                        }
                        onPositionChanged: if (drag.active)
                                               objectBlock.previewGeometry()
                        onDoubleClicked: {
                            if (!objectBlock.modelData.locked && (!objectBlock.modelData.kind
                                                                  || objectBlock.modelData.kind
                                                                  === "text")) {
                                richEditor.begin(objectBlock.index);
                            }
                        }
                        onReleased: {
                            if (!objectBlock.modelData.locked)
                                viewport.host.doc.updateElement(objectBlock.index, {
                                                                    x: viewport.host.snap
                                                                       ? Math.round(objectBlock.x
                                                                                    / 8) * 8 :
                                                                         objectBlock.x,
                                                                    y: viewport.host.snap
                                                                       ? Math.round(objectBlock.y
                                                                                    / 8) * 8 :
                                                                         objectBlock.y
                                                                });
                            editorRenderer.gestureId = "";
                        }
                        onCanceled: editorRenderer.gestureId = ""
                    }
                }
            }

            EditorResizeHandles {
                host: viewport.host
                renderer: editorRenderer
                coordinateSpace: artboard
                canvasScale: viewport.fitScale * viewport.host.zoom
            }
        }
    }
}
