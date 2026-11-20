pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: panel

    required property var store
    required property var controller
    required property var document
    required property var operatorWindow
    property string editingItemId: ""
    property int previewIndex: 0
    property string loadedItem: ""
    property bool itemDirty: false
    property bool loading: false
    property bool keyboardAllowed: true
    readonly property bool canNavigate: panel.opened && panel.store.live && !panel.textEntry
                                        && panel.keyboardAllowed && !newDialog.opened
    readonly property var previewSlide: store.selectedSlides[previewIndex] || ({})
    readonly property bool textEntry: {
        let f = operatorWindow.activeFocusItem;
        return !!f && (f.cursorPosition !== undefined || f.currentIndex !== undefined);
    }

    signal songsRequested
    signal bibleRequested
    signal mediaRequested
    signal templatesRequested
    signal outputsRequested
    signal editRequested(var slides, string itemId)

    function choosePresentationFile() {
        presentationFile.open();
    }

    function markItemDirty() {
        itemDirty = true;
        notesDebounce.restart();
    }

    function flushItem() {
        if (itemDirty && loadedItem === store.selectedItem.id) {
            store.setItemDetails(itemEditor.titleText, itemEditor.notesText);
            itemDirty = false;
        }
    }

    function loadItem() {
        loading = true;
        loadedItem = store.selectedItem.id || "";
        itemEditor.titleText = store.selectedItem.title || "";
        itemEditor.notesText = store.selectedItem.notes || "";
        itemDirty = false;
        previewIndex = Math.max(0, Math.min(previewIndex, store.selectedSlides.length - 1));
        loading = false;
    }

    function select(index) {
        flushItem();
        store.selected = index;
        previewIndex = 0;
        loadItem();
    }

    function takeLive() {
        flushItem();
        store.present(previewIndex);
    }

    function navigate(delta) {
        if (store.live)
            controller.advance(delta);
    }

    width: Math.min(1380, parent.width - 32)
    height: parent.height - 32
    x: (parent.width - width) / 2
    y: 16
    modal: true
    focus: true
    padding: 20

    background: Rectangle {
        color: "#191919"
        border.color: "#3B3B3B"
        radius: 12
    }

    onOpened: loadItem()
    onClosed: {
        flushItem();
        store.save();
    }

    Connections {
        function onChanged() {
            if (!panel.itemDirty)
                panel.loadItem();
        }

        target: panel.store
    }

    Shortcut {
        sequence: "Right"
        enabled: panel.canNavigate

        onActivated: panel.navigate(1)
    }

    Shortcut {
        sequence: "Space"
        enabled: panel.canNavigate

        onActivated: panel.navigate(1)
    }

    Shortcut {
        sequence: "Left"
        enabled: panel.canNavigate

        onActivated: panel.navigate(-1)
    }

    Shortcut {
        sequence: "Home"
        enabled: panel.canNavigate

        onActivated: panel.controller.advance(-panel.controller.liveIndex)
    }

    Shortcut {
        sequence: "End"
        enabled: panel.canNavigate

        onActivated: panel.controller.advance(panel.controller.liveCount - 1
                                              - panel.controller.liveIndex)
    }

    Shortcut {
        sequences: [StandardKey.Save]
        enabled: panel.opened && panel.keyboardAllowed && !newDialog.opened

        onActivated: {
            panel.flushItem();
            panel.store.save();
        }
    }

    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: panel.opened && panel.keyboardAllowed && !panel.textEntry && !newDialog.opened

        onActivated: panel.store.undo()
    }

    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: panel.opened && panel.keyboardAllowed && !panel.textEntry && !newDialog.opened

        onActivated: panel.store.redo()
    }

    Timer {
        id: notesDebounce

        interval: 450

        onTriggered: panel.flushItem()
    }

    FileDialog {
        id: presentationFile

        title: "Add a Church Deck presentation"
        nameFilters: ["Church Deck projects (*.churchdeck)"]

        onAccepted: panel.store.importPresentation(selectedFile)
    }

    FileDialog {
        id: exportFile

        title: "Export program"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "churchprogram"
        nameFilters: ["Church Deck programs (*.churchprogram)"]

        onAccepted: panel.store.exportProgram(selectedFile)
    }

    FileDialog {
        id: importFile

        title: "Import program"
        nameFilters: ["Church Deck programs (*.churchprogram)"]

        onAccepted: {
            panel.flushItem();
            panel.store.importProgram(selectedFile);
        }
    }

    Dialog {
        id: newDialog

        modal: true
        anchors.centerIn: parent
        width: 380
        title: "New program"

        background: Rectangle {
            color: "#252525"
            border.color: "#444444"
            radius: 8
        }
        contentItem: ColumnLayout {
            spacing: 12

            LibraryField {
                id: newTitle

                objectName: "newProgramTitle"
                placeholderText: "Sunday Worship"
                Layout.fillWidth: true
            }

            LibraryField {
                id: newDate

                placeholderText: "YYYY-MM-DD (optional)"
                Layout.fillWidth: true
            }

            Text {
                text: panel.store.status
                color: "#AAAAAA"
                font.pixelSize: 11
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            RowLayout {
                Item {
                    Layout.fillWidth: true
                }

                StudioButton {
                    text: "Cancel"

                    onClicked: newDialog.close()
                }

                StudioButton {
                    text: "Create program"
                    accented: true

                    onClicked: {
                        panel.flushItem();
                        if (panel.store.createProgram(newTitle.text, newDate.text))
                            newDialog.close();
                    }
                }
            }
        }

        onOpened: {
            newTitle.text = "Sunday Worship";
            newDate.text = Qt.formatDate(new Date(), "yyyy-MM-dd");
            newTitle.forceActiveFocus();
            newTitle.selectAll();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        RowLayout {
            Text {
                text: "Programs"
                color: "#EEEEEE"
                font.pixelSize: 21
                font.weight: Font.Medium
            }

            LibraryCombo {
                Layout.preferredWidth: 270
                model: panel.store.programs.map(p => ({
                    id: p.id,
                    label: p.title + (p.date ? " · " + p.date : "")
                }))
                textRole: "label"
                currentIndex: model.findIndex(p => p.id === panel.store.id)

                onActivated: index => {
                    panel.flushItem();
                    panel.store.openProgram(model[index].id);
                    panel.previewIndex = 0;
                }
            }

            StudioButton {
                text: "New program"
                iconName: "plus"

                onClicked: newDialog.open()
            }

            Item {
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Import"

                onClicked: importFile.open()
            }

            StudioButton {
                text: "Export"
                enabled: panel.store.id.length > 0

                onClicked: {
                    panel.flushItem();
                    exportFile.open();
                }
            }

            StudioButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        RowLayout {
            visible: panel.store.id.length > 0
            spacing: 10

            LibraryField {
                id: programTitle

                objectName: "programTitle"
                text: panel.store.title
                Layout.fillWidth: true
                placeholderText: "Program title"
            }

            LibraryField {
                id: programDate

                text: panel.store.date
                Layout.preferredWidth: 130
                placeholderText: "YYYY-MM-DD"
            }

            StudioButton {
                text: "Apply details"

                onClicked: panel.store.updateDetails(programTitle.text, programDate.text)
            }

            StudioButton {
                text: "Undo"
                enabled: panel.store.canUndo

                onClicked: {
                    panel.flushItem();
                    panel.store.undo();
                }
            }

            StudioButton {
                text: "Redo"
                enabled: panel.store.canRedo

                onClicked: {
                    panel.flushItem();
                    panel.store.redo();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            ProgramOrder {
                id: orderPane

                programView: panel
            }

            Rectangle {
                implicitWidth: 1
                Layout.fillHeight: true
                color: "#333333"
            }

            ProgramItemEditor {
                id: itemEditor

                programView: panel
            }

            Rectangle {
                implicitWidth: 1
                Layout.fillHeight: true
                color: "#333333"
            }

            ProgramLiveControls {
                id: liveControls

                programView: panel
            }
        }

        Text {
            visible: panel.store.warnings.length > 0
            text: "Missing media: " + panel.store.warnings.join(", ")
            color: "#E1AD80"
            Layout.fillWidth: true
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#333333"
        }

        RowLayout {
            Text {
                text: panel.store.status
                color: "#999999"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Text {
                text: panel.store.items.length + " service items"
                color: "#777777"
                font.pixelSize: 11
            }

            StudioButton {
                text: panel.store.dirty ? "Save program" : "Saved"
                enabled: panel.store.id.length > 0

                onClicked: {
                    panel.flushItem();
                    panel.store.save();
                }
            }
        }
    }
}
