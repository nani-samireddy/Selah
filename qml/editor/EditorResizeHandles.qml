pragma ComponentBehavior: Bound
import QtQuick

// A separate overlay keeps grips above text editing and overlapping layer hit areas.
// Geometry stays transient until release, so one drag produces one undo step.
Item {
    id: selection

    required property var host
    required property var renderer
    required property Item coordinateSpace
    required property real canvasScale
    readonly property var element: host.doc.selectedElement
    readonly property var geometry: renderer.gestureId === element.uid ? renderer.gestureGeometry :
                                                                         element

    property bool resizing: false
    property var initial: ({})
    property var preview: ({})
    property point pointerOrigin
    property int corner: 0
    property string resizeId: ""
    property real cosine: 1
    property real sine: 0
    readonly property real hitSize: 24 / canvasScale
    readonly property real markerSize: 7 / canvasScale

    function begin(handle, point) {
        if (element.locked || host.handTool || !element.uid)
            return;
        if (host.activeTextEditor)
            host.activeTextEditor.finish();
        selection.forceActiveFocus();
        initial = {
            x: element.x,
            y: element.y,
            width: element.width,
            height: element.height
        };
        preview = initial;
        resizeId = element.uid;
        pointerOrigin = point;
        corner = handle;
        const angle = (element.rotation || 0) * Math.PI / 180;
        cosine = Math.cos(angle);
        sine = Math.sin(angle);
        resizing = true;
    }

    function geometryFor(width, height) {
        const sx = corner % 2 ? 1 : -1, sy = corner > 1 ? 1 : -1;
        const dx = sx * (width - initial.width) / 2;
        const dy = sy * (height - initial.height) / 2;
        return {
            x: initial.x + initial.width / 2 + cosine * dx - sine * dy - width / 2,
            y: initial.y + initial.height / 2 + sine * dx + cosine * dy - height / 2,
            width: width,
            height: height
        };
    }

    function inBounds(rect) {
        // Match document geometry limits. Axis-aligned layers also stay on canvas.
        if (rect.x < 0 || rect.y < 0 || rect.x > 1880 || rect.y > 1050)
            return false;
        if (Math.abs(sine) < 0.00001 && cosine > 0.99999)
            return rect.x + rect.width <= Math.max(1920, initial.x + initial.width) && rect.y
                    + rect.height <= Math.max(1080, initial.y + initial.height);
        return true;
    }

    function updateResize(point) {
        if (!resizing || element.uid !== resizeId)
            return;
        const dx = point.x - pointerOrigin.x, dy = point.y - pointerOrigin.y;
        const sx = corner % 2 ? 1 : -1, sy = corner > 1 ? 1 : -1;
        let width = initial.width + sx * (cosine * dx + sine * dy);
        let height = initial.height + sy * (-sine * dx + cosine * dy);
        if (host.snap) {
            width = Math.round(width / 8) * 8;
            height = Math.round(height / 8) * 8;
        }
        width = Math.max(40, Math.min(1920, width));
        height = Math.max(30, Math.min(1080, height));
        let result = geometryFor(width, height);
        if (!inBounds(result)) {
            // Restrict the gesture along its path, preserving the opposite rotated corner.
            let low = 0, high = 1;
            for (let i = 0; i < 24; ++i) {
                const t = (low + high) / 2;
                const candidate = geometryFor(initial.width + (width - initial.width) * t,
                                              initial.height + (height - initial.height) * t);
                if (inBounds(candidate))
                    low = t;
                else
                    high = t;
            }
            result = geometryFor(initial.width + (width - initial.width) * low, initial.height + (height
                                                                                                  - initial.height)
                                 * low);
        }
        preview = result;
        renderer.gestureGeometry = result;
        renderer.gestureId = resizeId;
    }

    function finish() {
        if (!resizing)
            return;
        const patch = preview, uid = resizeId;
        resizing = false;
        if (element.uid === uid && !element.locked)
            host.doc.updateElement(host.doc.selected, patch);
        if (renderer.gestureId === uid)
            renderer.gestureId = "";
        resizeId = "";
    }

    function cancel() {
        if (renderer.gestureId === resizeId)
            renderer.gestureId = "";
        resizing = false;
        resizeId = "";
    }

    objectName: "editorSelectionHandles"
    x: geometry.x || 0
    y: geometry.y || 0
    width: geometry.width || 0
    height: geometry.height || 0
    rotation: element.rotation || 0
    visible: host.doc.selected >= 0 && element.visible !== false
    z: 20000

    onElementChanged: if (resizing && (element.uid !== resizeId || element.locked))
                          cancel()
    onVisibleChanged: if (!visible && resizing)
                          cancel()

    Rectangle {
        anchors.fill: parent
        anchors.margins: -6
        color: "transparent"
        border.color: "#CC8FE5"
        border.width: 1.3 / selection.canvasScale
    }

    Repeater {
        model: 4

        delegate: Item {
            id: grip

            required property int index

            objectName: "editorResizeHandle_" + index
            visible: !selection.element.locked && !selection.host.handTool
            width: selection.hitSize
            height: selection.hitSize
            x: (index % 2 ? selection.width + 6 : -6) - width / 2
            y: (index > 1 ? selection.height + 6 : -6) - height / 2

            Rectangle {
                anchors.centerIn: parent
                width: selection.markerSize
                height: width
                color: "#F7F2FA"
                border.color: "#A865C7"
                border.width: 1 / selection.canvasScale
            }

            MouseArea {
                anchors.fill: parent
                enabled: !selection.element.locked && !selection.host.handTool
                preventStealing: true
                cursorShape: grip.index === 0 || grip.index === 3 ? Qt.SizeFDiagCursor :
                                                                    Qt.SizeBDiagCursor

                onPressed: mouse => selection.begin(grip.index, mapToItem(selection.coordinateSpace,
                                                                          mouse.x, mouse.y))
                onPositionChanged: mouse => {
                    if (pressed)
                        selection.updateResize(mapToItem(selection.coordinateSpace, mouse.x,
                                                         mouse.y));
                }
                onReleased: selection.finish()
                onCanceled: selection.cancel()
            }
        }
    }
}
