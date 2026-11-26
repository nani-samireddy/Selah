pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQml.Models
import QtQuick.Layouts

// Frontmost layers appear first. The document remains the owner of stacking order.
StudioPanel {
    id: panel

    required property var host
    property bool listReady: false
    property bool collapsed: false

    // Reconcile by UID so editing text never destroys a row holding a drag gesture.
    function syncSelection() {
        const selected = host.doc.selected;
        layerView.currentIndex = selected < 0 ? -1 : layerRows.count - 1 - selected;
    }

    function syncLayers() {
        if (!listReady)
            return;
        const incoming = host.doc.elements.slice().reverse();
        let keys = [];
        for (let i = 0; i < layerRows.count; i++)
            keys.push(layerRows.get(i).key);
        for (let i = 0; i < incoming.length; i++) {
            const record = incoming[i], position = keys.indexOf(record.uid, i);
            if (position < 0) {
                layerRows.insert(i, {
                                     key: record.uid,
                                     record: record
                                 });
                keys.splice(i, 0, record.uid);
            } else {
                if (position !== i) {
                    layerRows.move(position, i, 1);
                    keys.splice(i, 0, keys.splice(position, 1)[0]);
                }
                if (JSON.stringify(layerRows.get(i).record) !== JSON.stringify(record))
                    layerRows.setProperty(i, "record", record);
            }
        }
        if (layerRows.count > incoming.length)
            layerRows.remove(incoming.length, layerRows.count - incoming.length);
        // Let ListView finish its model update before selecting a newly created row.
        Qt.callLater(panel.syncSelection);
    }

    x: 18
    y: 20
    width: 218
    height: collapsed ? 50 : Math.max(140, Math.min(520, parent.height - 40, 150
                                                    + host.doc.elements.length * 38))
    clip: true

    Behavior on height {
        NumberAnimation {
            duration: InteractionFeedback.duration
        }
    }

    Component.onCompleted: {
        listReady = true;
        syncLayers();
    }

    ListModel {
        id: layerRows

        dynamicRoles: true
    }

    Connections {
        function onChanged() {
            panel.syncLayers();
        }

        function onSelectionChanged() {
            Qt.callLater(panel.syncSelection);
        }

        target: panel.host.doc
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: "Layers"
                color: LibraryStyle.text
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            LayerAction {
                visible: !panel.collapsed
                iconName: "chevron-up"
                hint: "Bring selected layer forward"
                enabled: panel.host.doc.selected >= 0 && panel.host.doc.selected
                         < panel.host.doc.elements.length - 1

                onClicked: panel.host.doc.moveSelected(1)
            }

            LayerAction {
                visible: !panel.collapsed
                iconName: "chevron-down"
                hint: "Send selected layer backward"
                enabled: panel.host.doc.selected > 0

                onClicked: panel.host.doc.moveSelected(-1)
            }

            LayerAction {
                visible: !panel.collapsed
                iconName: "copy"
                hint: "Duplicate selected layer"
                enabled: panel.host.doc.selected >= 0

                onClicked: panel.host.doc.duplicateSelected()
            }

            LayerAction {
                visible: !panel.collapsed
                iconName: "trash-2"
                hint: "Delete selected layer"
                enabled: panel.host.doc.selected >= 0

                onClicked: panel.host.doc.deleteSelected()
            }

            LayerAction {
                visible: !panel.collapsed
                iconName: "plus"
                hint: "Add text layer"

                onClicked: panel.host.doc.addText()
            }

            LayerAction {
                objectName: "toggleLayersPanel"
                iconName: panel.collapsed ? "chevron-down" : "chevron-up"
                hint: panel.collapsed ? "Expand Layers" : "Collapse Layers"
                Accessible.name: hint

                onClicked: panel.collapsed = !panel.collapsed
            }
        }

        ColumnLayout {
            objectName: "layersPanelBody"
            visible: !panel.collapsed
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            StudioRule {
            }

            RowLayout {
                Layout.fillWidth: true

                StudioIcon {
                    name: "layers"
                    tone: LibraryStyle.muted
                }

                Text {
                    text: panel.host.doc.slides[panel.host.doc.currentSlide].name
                    textFormat: Text.PlainText
                    color: LibraryStyle.muted
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                StudioMicroLabel {
                    text: "16:9"
                    font.pixelSize: 9
                    font.letterSpacing: 0
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: layerView

                    objectName: "layerOrderList"
                    anchors.fill: parent
                    model: layerRows
                    currentIndex: -1
                    spacing: 2
                    clip: true
                    cacheBuffer: layerDrag.dragging ? contentHeight : height
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar {
                    }
                    delegate: Rectangle {
                        id: row

                        required property var record
                        required property int index
                        readonly property var modelData: record
                        readonly property int sourceIndex: panel.host.doc.elements.length - 1
                                                           - index
                        readonly property bool selected: panel.host.doc.selected === sourceIndex

                        width: layerView.width
                        height: 36
                        radius: 5
                        color: selected ? LibraryStyle.tone("#343434") : mouse.containsMouse
                                          ? LibraryStyle.hover : "transparent"
                        border.width: selected ? 1 : 0
                        border.color: LibraryStyle.border
                        opacity: layerDrag.dragging && layerDrag.sourceId === modelData.uid ? 0.3 :
                                                                                              1
                        Accessible.role: Accessible.ListItem
                        Accessible.name: row.modelData.name
                        Accessible.selected: row.selected

                        Accessible.onPressAction: panel.host.doc.selected = row.sourceIndex

                        MouseArea {
                            id: mouse

                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

                            onPressed: event => layerDrag.arm(row.index, mapToItem(layerDrag,
                                                                                   event.x, event.y))
                            onPositionChanged: event => {
                                if (pressed)
                                    layerDrag.update(mapToItem(layerDrag, event.x, event.y));
                            }
                            onReleased: layerDrag.finish()
                            onCanceled: layerDrag.cancel()
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 4
                            spacing: 7

                            StudioIcon {
                                name: row.modelData.kind === "image" ? "image" : row.modelData.kind
                                                                       === "video" ? "play" : "type"
                                tone: LibraryStyle.muted
                            }

                            Text {
                                text: row.modelData.name
                                textFormat: Text.PlainText
                                color: row.selected ? LibraryStyle.text : LibraryStyle.muted
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            LayerAction {
                                iconName: row.modelData.visible ? "eye" : "eye-off"
                                hint: row.modelData.visible ? "Hide " + row.modelData.name :
                                                              "Show " + row.modelData.name

                                onClicked: panel.host.doc.updateElement(row.sourceIndex, {
                                                                            visible:
                                                                                !row.modelData.visible
                                                                        })
                            }
                        }
                    }

                    Keys.onUpPressed: panel.host.doc.selected = panel.host.doc.selected < 0 ? count
                                                                                              - 1 : Math.min(
                                                                                                  count - 1,
                                                                                                  panel.host.doc.selected
                                                                                                  + 1)
                    Keys.onDownPressed: panel.host.doc.selected = panel.host.doc.selected < 0 ? count
                                                                                                - 1 : Math.max(
                                                                                                    0, panel.host.doc.selected
                                                                                                    - 1)
                }

                LayerOrderDrag {
                    id: layerDrag

                    anchors.fill: parent
                    view: layerView
                    host: panel.host
                }
            }

            StudioRule {
            }

            LibraryButton {
                objectName: "canvasBackgroundButton"
                text: "Background"
                iconName: "image"
                quiet: true
                Layout.fillWidth: true
                hint: "Canvas background settings"

                onClicked: panel.host.selectCanvasSettings()
            }
        }
    }

    component LayerAction: StudioButton {
        implicitWidth: 24
        implicitHeight: 26
        quiet: true
    }
}
