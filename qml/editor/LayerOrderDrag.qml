pragma ComponentBehavior: Bound
import QtQuick

// Preview against the reversed layer list; commit one stable-ID move on drop.
Item {
    id: gesture

    required property var host
    required property var view
    property var platformHints: Qt.styleHints
    property string sourceId: ""
    property string sourceName: ""
    property int slideIndex: -1
    property bool dragging: false
    property int slot: -1
    property point origin
    property point pointer
    readonly property real markerY: {
        if (!dragging || slot < 0 || !view.count)
            return -100;
        const row = view.itemAtIndex(Math.min(slot, view.count - 1));
        return row ? row.y + (slot === view.count ? row.height + view.spacing / 2 : -view.spacing
                                                    / 2) - view.contentY : -100;
    }

    function arm(index, point) {
        const item = index >= 0 && index < view.count ? view.model.get(index).record : null;
        if (!item)
            return;
        if (host.activeTextEditor)
            host.activeTextEditor.finish();
        const source = host.doc.elements.findIndex(layer => layer.uid === item.uid);
        if (source < 0)
            return;
        host.doc.selected = source;
        sourceId = item.uid;
        sourceName = item.name;
        slideIndex = host.doc.currentSlide;
        origin = point;
        pointer = point;
        slot = index;
        view.forceActiveFocus();
    }

    function update(point) {
        if (!sourceId)
            return;
        if (!dragging && Math.hypot(point.x - origin.x, point.y - origin.y)
                < platformHints.startDragDistance)
            return;
        dragging = true;
        pointer = point;
        locate();
    }

    function locate() {
        const y = view.contentY + Math.max(0, Math.min(view.height - 1, pointer.y));
        let index = view.indexAt(8, y);
        if (index < 0 && y < view.contentHeight)
            index = view.indexAt(8, y + view.spacing);
        if (index < 0) {
            slot = y <= 0 ? 0 : view.count;
            return;
        }
        const row = view.itemAtIndex(index);
        slot = index + (row && y > row.y + row.height / 2 ? 1 : 0);
    }

    function finish() {
        const moved = dragging;
        const inside = pointer.x >= 0 && pointer.x <= width && pointer.y >= 0 && pointer.y
              <= height;
        const id = sourceId, target = slot, slide = slideIndex;
        cancel();
        if (!moved || !inside || host.doc.currentSlide !== slide)
            return;
        const count = host.doc.elements.length;
        const from = host.doc.elements.findIndex(layer => layer.uid === id);
        if (from < 0 || target < 0)
            return;
        const visualFrom = count - 1 - from;
        const visualTo = target > visualFrom ? target - 1 : target;
        if (host.doc.moveElementTo(from, count - 1 - visualTo))
            InteractionFeedback.play("drop");
        view.forceActiveFocus();
    }

    function cancel() {
        dragging = false;
        sourceId = "";
        slot = -1;
    }

    objectName: "layerOrderDrag"
    clip: true

    Shortcut {
        sequence: "Escape"
        enabled: gesture.dragging

        onActivated: gesture.cancel()
    }

    Connections {
        function onChanged() {
            if (gesture.sourceId)
                gesture.cancel();
        }

        target: gesture.host.doc
    }

    Timer {
        interval: 25
        repeat: true
        running: gesture.dragging

        onTriggered: {
            const step = gesture.pointer.y < 30 ? -10 : gesture.pointer.y > gesture.height - 30
                                                  ? 10 : 0;
            if (step) {
                gesture.view.contentY = Math.max(0, Math.min(Math.max(0, gesture.view.contentHeight
                                                                      - gesture.view.height),
                                                             gesture.view.contentY + step));
                gesture.locate();
            }
        }
    }

    Rectangle {
        visible: gesture.dragging
        y: gesture.markerY
        width: parent.width
        height: 2
        radius: 1
        color: LibraryStyle.dark ? "#C2B4CA" : "#79528F"
    }

    Rectangle {
        visible: gesture.dragging
        x: 4
        y: Math.max(0, Math.min(gesture.height - height, gesture.pointer.y - height / 2))
        width: parent.width - 8
        height: 34
        radius: 5
        color: LibraryStyle.panel
        opacity: 0.95
        border.color: LibraryStyle.focus

        Text {
            anchors.fill: parent
            anchors.margins: 8
            text: gesture.sourceName
            textFormat: Text.PlainText
            color: LibraryStyle.text
            font.pixelSize: 11
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
}
