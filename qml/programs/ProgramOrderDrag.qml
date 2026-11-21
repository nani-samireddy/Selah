pragma ComponentBehavior: Bound
import QtQuick

// Preview a reorder while dragging; publish one undoable move only on release.
Item {
    id: dragState

    property var platformHints: Qt.styleHints
    required property var view
    required property var programView
    property bool dragging: false
    property string sourceId: ""
    property string programId: ""
    property string sourceTitle: ""
    property int slot: -1
    property real pointerY: 0
    property point pointer
    property point origin
    readonly property real markerY: {
        if (!dragging || slot < 0)
            return -100;
        const row = slot < view.count ? view.itemAtIndex(slot) : view.itemAtIndex(view.count - 1);
        if (!row)
            return -100;
        return row.y + (slot === view.count ? row.height + view.spacing / 2 : -view.spacing / 2)
                - view.contentY;
    }

    function arm(index, point) {
        const item = programView.store.items[index];
        if (!item)
            return;
        programView.suspendItemSave();
        sourceId = item.id;
        programId = programView.store.id;
        sourceTitle = item.title;
        origin = point;
        pointer = point;
        pointerY = point.y;
        slot = index;
    }

    function update(point) {
        if (!sourceId)
            return;
        if (programId !== programView.store.id) {
            cancel();
            return;
        }
        if (!dragging && Math.hypot(point.x - origin.x, point.y - origin.y)
                < platformHints.startDragDistance)
            return;
        dragging = true;
        pointer = point;
        pointerY = point.y;
        locate();
    }

    function locate() {
        const y = view.contentY + Math.max(0, Math.min(view.height - 1, pointerY));
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
        const didDrag = dragging;
        const inside = pointer.x >= 0 && pointer.x <= view.width && pointer.y >= 0 && pointer.y
              <= view.height;
        const id = sourceId, targetSlot = slot, originalProgram = programId;
        cancel();
        if (!didDrag)
            return false;
        if (!inside)
            return true;
        programView.flushItem();
        if (programView.store.id !== originalProgram)
            return true;
        const from = programView.store.items.findIndex(item => item.id === id);
        if (from >= 0 && targetSlot >= 0)
            programView.store.moveItemTo(from, targetSlot > from ? targetSlot - 1 : targetSlot);
        programView.focusOrder();
        return true;
    }

    function cancel() {
        if (sourceId.length && programView)
            programView.resumeItemSave();
        dragging = false;
        sourceId = "";
        slot = -1;
    }

    objectName: "programOrderDrag"
    clip: true

    Shortcut {
        sequence: "Escape"
        enabled: dragState.dragging

        onActivated: dragState.cancel()
    }

    Timer {
        interval: 25
        repeat: true
        running: dragState.dragging

        onTriggered: {
            const edge = 36;
            let step = 0;
            if (dragState.pointerY < edge)
                step = -10;
            else if (dragState.pointerY > dragState.view.height - edge)
                step = 10;
            if (step) {
                const maximum = Math.max(0, dragState.view.contentHeight - dragState.view.height);
                dragState.view.contentY = Math.max(0, Math.min(maximum, dragState.view.contentY
                                                               + step));
                dragState.locate();
            }
        }
    }

    Rectangle {
        visible: dragState.dragging
        x: 0
        y: dragState.markerY
        width: dragState.view.width - 8
        height: 2
        radius: 1
        color: "#C2B4CA"
    }

    Rectangle {
        visible: dragState.dragging
        x: 8
        y: Math.max(0, Math.min(dragState.height - height, dragState.pointerY - height / 2))
        width: dragState.width - 24
        height: 36
        radius: 6
        color: "#E82B292D"
        border.color: "#716779"

        Text {
            anchors.fill: parent
            anchors.margins: 10
            text: dragState.sourceTitle
            textFormat: Text.PlainText
            font.pixelSize: 12
            color: LibraryStyle.tone("#EEEEEE")
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
