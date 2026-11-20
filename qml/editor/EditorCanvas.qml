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

    clip: true

    MouseArea {
        property point origin
        property point panOrigin

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        cursorShape: viewport.host.handTool ? Qt.OpenHandCursor : Qt.ArrowCursor

        onPressed: mouse => {
            if (viewport.host.handTool || mouse.button === Qt.MiddleButton) {
                origin = Qt.point(mouse.x, mouse.y);
                panOrigin = Qt.point(viewport.host.panX, viewport.host.panY);
            } else
                viewport.host.doc.selected = -1;
        }
        onPositionChanged: mouse => {
            if (pressed && (viewport.host.handTool || pressedButtons & Qt.MiddleButton)) {
                viewport.host.panX = panOrigin.x + mouse.x - origin.x;
                viewport.host.panY = panOrigin.y + mouse.y - origin.y;
            }
        }
        onWheel: wheel => {
            viewport.host.zoom = Math.max(0.4, Math.min(2.5, viewport.host.zoom + (
                                                            wheel.angleDelta.y > 0 ? 0.08 :
                                                                                     -0.08)));
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
            color: "#949494"
            font.pixelSize: 10

            anchors {
                left: parent.left
                bottom: parent.top
                bottomMargin: 12
            }
        }

        Text {
            text: "1920 × 1080"
            color: "#686868"
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

            width: 1920
            height: 1080
            scale: viewport.fitScale * viewport.host.zoom
            transformOrigin: Item.TopLeft

            SlideRenderer {
                id: editorRenderer

                objectName: "editorSlideRenderer"
                elements: viewport.host.doc.elements
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

                onClicked: viewport.host.doc.selected = -1
            }

            Repeater {
                model: viewport.host.doc.elements

                delegate: Item {
                    id: objectBlock

                    required property var modelData
                    required property int index
                    property bool editing: false
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

                    TextEdit {
                        id: slideText

                        textFormat: TextEdit.PlainText
                        anchors.fill: parent
                        visible: objectBlock.editing
                        text: objectBlock.modelData.text
                        color: objectBlock.modelData.color
                        font.family: objectBlock.modelData.fontFamily
                        font.pixelSize: objectBlock.modelData.fontSize
                        wrapMode: TextEdit.Wrap
                        horizontalAlignment: objectBlock.modelData.alignment === "center"
                                             ? TextEdit.AlignHCenter :
                                               objectBlock.modelData.alignment === "right"
                                               ? TextEdit.AlignRight : TextEdit.AlignLeft
                        readOnly: !objectBlock.editing
                        selectByMouse: objectBlock.editing
                        selectionColor: "#7F4B93"
                        clip: true

                        onActiveFocusChanged: if (!activeFocus && objectBlock.editing) {
                                                  objectBlock.editing = false;
                                                  viewport.host.editingId = "";
                                                  viewport.host.doc.updateElement(objectBlock.index,
                                                                                  {
                                                                                      text: text
                                                                                  });
                                              }
                        Keys.onEscapePressed: {
                            objectBlock.editing = false;
                            viewport.host.editingId = "";
                            viewport.host.doc.updateElement(objectBlock.index, {
                                                                text: text
                                                            });
                            viewport.forceActiveFocus();
                        }
                    }

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

                        onPressed: viewport.host.doc.selected = objectBlock.index
                        onPositionChanged: if (drag.active)
                                               objectBlock.previewGeometry()
                        onDoubleClicked: {
                            if (!objectBlock.modelData.locked && (!objectBlock.modelData.kind
                                                                  || objectBlock.modelData.kind
                                                                  === "text")) {
                                objectBlock.editing = true;
                                viewport.host.editingId = objectBlock.modelData.uid;
                                slideText.forceActiveFocus();
                                slideText.selectAll();
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

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -6
                        visible: objectBlock.selected
                        color: "transparent"
                        border.color: "#CC8FE5"
                        border.width: 1.3 / (viewport.fitScale * viewport.host.zoom)
                    }

                    Repeater {
                        model: objectBlock.selected ? 4 : 0

                        delegate: Rectangle {
                            required property int index
                            property real handleSize: 7 / (viewport.fitScale * viewport.host.zoom)

                            width: handleSize
                            height: handleSize
                            x: (index % 2 ? objectBlock.width + 6 : -6) - width / 2
                            y: (index > 1 ? objectBlock.height + 6 : -6) - height / 2
                            color: "#F7F2FA"
                            border.color: "#A865C7"
                            border.width: 1 / (viewport.fitScale * viewport.host.zoom)
                        }
                    }

                    Rectangle {
                        visible: objectBlock.selected && !objectBlock.modelData.locked
                        width: 22 / (viewport.fitScale * viewport.host.zoom)
                        height: width
                        x: parent.width - width / 2
                        y: parent.height - height / 2
                        color: "transparent"

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.SizeFDiagCursor

                            onPositionChanged: mouse => {
                                if (pressed) {
                                    let p = mapToItem(artboard, mouse.x, mouse.y);
                                    objectBlock.width = Math.max(80, Math.min(1920 - objectBlock.x,
                                                                              p.x - objectBlock.x));
                                    objectBlock.height = Math.max(40, Math.min(1080 - objectBlock.y,
                                                                               p.y - objectBlock.y));
                                    objectBlock.previewGeometry();
                                }
                            }
                            onReleased: {
                                viewport.host.doc.updateElement(objectBlock.index, {
                                                                    width: objectBlock.width,
                                                                    height: objectBlock.height
                                                                });
                                editorRenderer.gestureId = "";
                            }
                            onCanceled: editorRenderer.gestureId = ""
                        }
                    }
                }
            }
        }
    }
}
