pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: panel

    property bool workspaceMode: false
    property bool editorOpen: false
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
    readonly property bool canNavigate: !panel.workspaceMode && panel.opened && panel.store.live &&
                                        !panel.textEntry && panel.keyboardAllowed &&
                                        !newDialog.opened
    readonly property var previewSlide: store.selectedSlides[previewIndex] || ({})
    readonly property bool textEntry: {
        let f = operatorWindow.activeFocusItem;
        return !!f && (f.cursorPosition !== undefined || f.currentIndex !== undefined);
    }
    readonly property bool dialogOpen: newDialog.opened || importFile.visible || exportFile.visible
                                       || bundleFile.visible || editorHeader.dialogOpen

    signal presentRequested
    signal songsRequested
    signal bibleRequested
    signal mediaRequested
    signal presentationsRequested
    signal templatesRequested
    signal outputsRequested
    signal editRequested(var slides, string itemId)

    function showAllPrograms() {
        flushItem();
        if (store.save())
            editorOpen = false;
    }

    function showEditor() {
        if (store.id.length > 0) {
            editorOpen = true;
            loadItem();
        }
    }

    function openProgram(programId) {
        flushItem();
        if (store.openProgram(programId)) {
            previewIndex = 0;
            showEditor();
        }
    }

    function showNewProgramDialog() {
        flushItem();
        newDialog.open();
    }

    function showImportDialog() {
        flushItem();
        importFile.open();
    }

    function showExportDialog() {
        flushItem();
        if (store.id.length > 0)
            exportFile.open();
    }

    function showBundleExportDialog() {
        flushItem();
        if (store.id.length > 0)
            bundleFile.open();
    }

    function choosePresentationFile() {
        presentationFile.open();
    }

    function suspendItemSave() {
        notesDebounce.stop();
    }

    function resumeItemSave() {
        if (itemDirty)
            notesDebounce.restart();
    }

    function focusOrder() {
        contentItem.forceActiveFocus();
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
        if (store.present(previewIndex) && workspaceMode)
            presentRequested();
    }

    function navigate(delta) {
        if (store.live)
            controller.advance(delta);
    }

    closePolicy: workspaceMode ? Popup.NoAutoClose : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: workspaceMode ? parent.width : Math.min(1380, parent.width - 32)
    height: workspaceMode ? parent.height : parent.height - 32
    x: workspaceMode ? 0 : (parent.width - width) / 2
    y: workspaceMode ? 0 : 16
    modal: !workspaceMode
    focus: true
    padding: 20

    background: Rectangle {
        color: LibraryStyle.tone("#191919")
        border.width: panel.workspaceMode ? 0 : 1
        border.color: LibraryStyle.tone("#3B3B3B")
        radius: panel.workspaceMode ? 0 : 12
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

    Timer {
        id: notesDebounce

        interval: 450

        onTriggered: panel.flushItem()
    }

    FileDialog {
        id: presentationFile

        title: "Add a Selah presentation"
        nameFilters: ["Selah projects (*.churchdeck)"]

        onAccepted: panel.store.importPresentation(selectedFile)
    }

    FileDialog {
        id: exportFile

        title: "Export program"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "churchprogram"
        nameFilters: ["Selah programs (*.churchprogram)"]

        onAccepted: panel.store.exportProgram(selectedFile)
    }

    FileDialog {
        id: bundleFile

        title: "Export portable program with media"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "selahbundle"
        nameFilters: ["Portable Selah programs (*.selahbundle)"]

        onAccepted: panel.store.exportBundle(selectedFile)
    }

    FileDialog {
        id: importFile

        title: "Import program"
        nameFilters: ["Selah programs (*.churchprogram *.selahbundle)"]

        onAccepted: {
            panel.flushItem();
            if (panel.store.importProgram(selectedFile))
                panel.showEditor();
        }
    }

    Connections {
        function onChanged() {
            if (panel.store.bundleBusy)
                bundleProgress.open();
            else
                bundleProgress.close();
        }

        target: panel.store
    }

    Popup {
        id: bundleProgress

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 440
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 24

        contentItem: ColumnLayout {
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: panel.store.status
                color: LibraryStyle.text
                wrapMode: Text.Wrap
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 100
                value: panel.store.bundleProgress
                indeterminate: value === 0
            }

            LibraryButton {
                text: "Cancel"

                onClicked: panel.store.cancelBundle()
            }
        }
    }

    ProgramDetailsDialog {
        id: newDialog

        store: panel.store
        creating: true

        onSaved: panel.showEditor()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        ProgramBrowser {
            visible: !panel.editorOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            store: panel.store

            onProgramChosen: programId => panel.openProgram(programId)
            onCreateRequested: panel.showNewProgramDialog()
            onImportRequested: panel.showImportDialog()
        }

        ProgramEditorHeader {
            id: editorHeader

            visible: panel.editorOpen
            programView: panel

            onBackRequested: panel.showAllPrograms()
        }

        RowLayout {
            visible: panel.editorOpen
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
                color: LibraryStyle.tone("#333333")
            }

            ProgramItemEditor {
                id: itemEditor

                programView: panel
            }

            Rectangle {
                visible: !panel.workspaceMode
                implicitWidth: 1
                Layout.fillHeight: true
                color: LibraryStyle.tone("#333333")
            }

            ProgramLiveControls {
                id: liveControls

                visible: !panel.workspaceMode
                programView: panel
            }
        }

        Text {
            visible: panel.editorOpen && panel.store.warnings.length > 0
            text: "Missing media: " + panel.store.warnings.join(", ")
            color: "#E1AD80"
            Layout.fillWidth: true
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
}
