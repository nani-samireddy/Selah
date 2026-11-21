pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

ApplicationWindow {
    id: app

    required property var fonts
    required property var doc
    required property var library
    required property var content
    required property var programs
    property string startupWorkspace: "programs"
    property string activeWorkspace: "design"
    property string librarySection: "songs"
    property string libraryDestination: "program"
    property bool fontPickerOpen: false
    readonly property bool settingsOpen: savePresentationDialog.opened || fontPickerOpen
                                         || outputPanel.opened || ndiPanel.opened
                                         || remotePanel.opened || saveTemplateDialog.opened
    property string programEditingItem: ""
    required property var outputs
    required property var presentations
    property alias editingPresentationId: customDraft.recordId
    property alias customPresentationTitle: customDraft.title
    required property var templates
    property alias editingTemplateId: customDraft.templateId
    required property var remote
    required property var ndi
    property var element: app.doc.selectedElement
    property alias templateMode: customDraft.templateEditing
    property string editingId: ""
    property var activeTextEditor: null
    property bool videosPlaying: false
    property bool snap: true
    property bool handTool: false
    property real zoom: 1
    property real panX: 0
    property real panY: 0
    property string toast: ""
    property bool libraryImportOnly: false
    readonly property bool editorShortcutsAllowed: activeWorkspace === "design" && !fontPickerOpen
                                                   && !programPanel.opened && !templatePanel.opened
                                                   && !saveTemplateDialog.opened &&
                                                   !songPanel.opened && !biblePanel.opened &&
                                                   !libraryPanel.opened && !outputPanel.opened &&
                                                   !ndiPanel.opened && !remotePanel.opened

    function formatText(patch) {
        if (app.activeTextEditor)
            app.activeTextEditor.applyFormat(patch);
        else
            app.doc.formatText(patch);
    }

    function update(key, value) {
        if (app.doc.selected >= 0) {
            if (app.selectedValue("kind", "text") === "text" && ["fontFamily", "fontSize", "color",
                                                                 "alignment", "fontWeight", "italic",
                                                                 "underline", "strikethrough",
                                                                 "letterSpacing", "lineHeight",
                                                                 "lineHeightUnit",
                                                                 "paragraphSpacing",
                                                                 "paragraphIndent", "textCase",
                                                                 "verticalAlignment", "textSizing",
                                                                 "wrap", "clipText", "baseline",
                                                                 "listStyle", "listIndent",
                                                                 "fontFeatures", "fontAxes",
                                                                 "maxLines", "ellipsis"].includes(
                        key)) {
                let style = {};
                style[key] = value;
                if (app.activeTextEditor)
                    app.activeTextEditor.applyFormat(style);
                else
                    app.doc.formatText(style);
                return;
            }
            let patch = {};
            patch[key] = value;
            app.doc.updateElement(app.doc.selected, patch);
        }
    }

    function selectedValue(key, fallback) {
        if (activeTextEditor && ["fontFamily", "fontSize", "fontWeight", "italic", "underline",
                                 "strikethrough", "color", "letterSpacing", "baseline", "alignment",
                                 "paragraphSpacing", "paragraphIndent", "lineHeight",
                                 "lineHeightUnit", "textCase", "listStyle", "listIndent",
                                 "fontFeatures", "fontAxes"].includes(key)
                && activeTextEditor.style[key] !== undefined)
            return activeTextEditor.style[key];
        return element[key] === undefined ? fallback : element[key];
    }

    function rendererProbe() {
        return workspace.rendererProbe();
    }

    function editCustomPresentation(definition) {
        if (activeTextEditor)
            activeTextEditor.finish();
        if (!doc.replaceSlides(definition.slides))
            return;
        editingTemplateId = "";
        programEditingItem = "";
        templateMode = false;
        editingPresentationId = definition.id;
        customPresentationTitle = definition.title;
        doc.saveProject();
        switchWorkspace("design");
    }

    function saveCustomPresentation() {
        savePresentationDialog.createBlank = false;
        savePresentationDialog.open();
    }

    function closeContentPanels() {
        programPanel.flushItem();
        programPanel.close();
        songPanel.close();
        biblePanel.close();
        libraryPanel.close();
        templatePanel.close();
        customSlidesPanel.close();
    }

    function switchWorkspace(name) {
        if (!["programs", "libraries", "design", "present"].includes(name))
            return;
        if (app.activeTextEditor)
            app.activeTextEditor.finish();
        app.contentItem.forceActiveFocus();
        closeContentPanels();
        activeWorkspace = name;
        if (name === "programs")
            Qt.callLater(() => {
                if (activeWorkspace === "programs")
                    programPanel.open();
            });
        if (name === "libraries")
            Qt.callLater(() => {
                if (activeWorkspace === "libraries")
                    showLibrarySection(librarySection);
            });
    }

    function refreshLibraryTarget() {
        let target = libraryDestination === "program" && programs.id ? programs : null;
        songPanel.programTarget = target;
        biblePanel.programTarget = target;
        libraryPanel.programTarget = target;
    }

    function showLibrarySection(section) {
        songPanel.close();
        biblePanel.close();
        libraryPanel.close();
        templatePanel.close();
        customSlidesPanel.close();
        librarySection = section;
        refreshLibraryTarget();
        if (section === "custom") {
            customSlidesPanel.programTarget = programs;
            customSlidesPanel.open();
        }
        if (section === "songs")
            songPanel.open();
        if (section === "bible")
            biblePanel.open();
        if (section === "media")
            libraryPanel.open();
        if (section === "templates") {
            templatePanel.programTarget = null;
            templatePanel.open();
        }
    }

    function browseLibrary(section) {
        librarySection = section;
        switchWorkspace("libraries");
    }

    function showSongs() {
        browseLibrary("songs");
    }

    function showBible() {
        browseLibrary("bible");
    }

    function showLibrary() {
        if (activeWorkspace === "design") {
            libraryPanel.programTarget = null;
            libraryPanel.open();
        } else
            browseLibrary("media");
    }

    function showTemplates(target) {
        if (target) {
            templatePanel.programTarget = target;
            templatePanel.open();
        } else
            browseLibrary("templates");
    }

    function contentAdded(toProgram, embedded) {
        app.notify(toProgram ? "Added to " + programs.title : "Added to the Design presentation");
        if (embedded)
            switchWorkspace(toProgram ? "programs" : "design");
    }

    function editTemplate(definition) {
        if (app.activeTextEditor)
            app.activeTextEditor.finish();
        if (app.doc.replaceSlides([definition.slide])) {
            programPanel.close();
            app.editingPresentationId = "";
            app.customPresentationTitle = "";
            app.editingTemplateId = definition.id;
            app.programEditingItem = "";
            app.templateMode = true;
            app.switchWorkspace("design");
        }
    }

    function saveCurrentTemplate() {
        saveTemplateDialog.open();
    }

    function showPrograms() {
        switchWorkspace("programs");
    }

    function showOutputs() {
        outputPanel.open();
    }

    function showRemote() {
        outputPanel.close();
        remotePanel.open();
    }

    function showNdi() {
        outputPanel.close();
        ndiPanel.open();
    }

    function showBackgroundPicker() {
        backgroundColor.open();
    }

    function showTextPicker() {
        textColor.open();
    }

    function openPresentation() {
        openDialog.open();
    }

    function exportPresentation() {
        exportDialog.open();
    }

    function notify(message) {
        toast = message;
        toastTimer.restart();
    }

    function preview() {
        outputWindow.contentSnapshot = app.doc.elements;
        outputWindow.slideColor = app.doc.background();
        outputWindow.backgroundOpacity = app.doc.ambientOpacity;
        outputWindow.playing = true;
        outputWindow.show();
        outputWindow.raise();
    }

    width: 1440
    height: 940
    minimumWidth: 1120
    minimumHeight: 760
    // Extend the toolbar beneath native caption controls while retaining the OS window frame.
    flags: Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint
    visible: true
    title: "Selah — " + (app.programs.title || "Presentation")
    color: LibraryStyle.tone("#111111")
    palette.window: LibraryStyle.tone("#171717")
    palette.text: LibraryStyle.tone("#DDDDDD")
    palette.base: LibraryStyle.tone("#252525")
    palette.button: LibraryStyle.tone("#252525")
    palette.buttonText: LibraryStyle.tone("#DDDDDD")
    font.family: LibraryStyle.fontFamily

    header: WorkspaceNavigation {
        host: app
    }
    footer: Rectangle {
        height: 30
        color: LibraryStyle.tone("#171717")

        Rectangle {
            width: parent.width
            height: 1
            color: LibraryStyle.tone("#2A2A2A")
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Text {
                text: app.activeWorkspace === "design" ? (app.templateMode
                                                          ? "Template design · Reusable layout" :
                                                            "Design · Draft changes stay off live output") :
                                                         app.activeWorkspace === "programs"
                                                         ? "Prepare your service · Select content, arrange, then present" :
                                                           app.activeWorkspace === "libraries"
                                                           ? "Your content · Available offline on this computer" :
                                                             "Present · Controls affect the live output"
                color: LibraryStyle.tone("#777777")
                font.pixelSize: 10
                Layout.fillWidth: true
            }

            Text {
                text: app.outputs.liveIndex >= 0 ? "LIVE  /  " + app.outputs.liveTitle + "  ·  "
                                                   + app.outputs.status : "Outputs stopped"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 10
            }
        }
    }

    Component.onCompleted: {
        app.outputs.bindWindows(audienceOutput, stageOutput, app);
        app.switchWorkspace(app.startupWorkspace);
    }
    onClosing: {
        if (app.activeTextEditor)
            app.activeTextEditor.finish();
        programPanel.flushItem();
        app.programs.save();
        workspace.forceActiveFocus();
        app.doc.saveProject();
        app.outputs.stop("audience");
        app.outputs.stop("stage");
        app.ndi.stop();
        app.remote.stop();
    }

    Settings {
        id: customDraft

        property string templateId: ""
        property bool templateEditing: false
        property string recordId: ""
        property string title: ""

        category: "EditorContext"
    }

    Binding {
        target: InteractionFeedback
        property: "presentationMode"
        value: app.activeWorkspace === "present"
    }

    Timer {
        id: toastTimer

        interval: 3200

        onTriggered: app.toast = ""
    }

    Connections {
        function onImportFailed(message) {
            app.notify(message);
        }

        target: app.doc
    }

    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: app.editorShortcutsAllowed

        onActivated: app.doc.undo()
    }

    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: app.editorShortcutsAllowed

        onActivated: app.doc.redo()
    }

    Shortcut {
        sequences: [StandardKey.Save]
        enabled: app.editorShortcutsAllowed

        onActivated: {
            if (app.templateMode)
                app.saveCurrentTemplate();
            else {
                app.saveCustomPresentation();
            }
        }
    }

    Shortcut {
        sequences: [StandardKey.Open]
        enabled: app.editorShortcutsAllowed

        onActivated: openDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+Shift+S"
        enabled: app.editorShortcutsAllowed

        onActivated: exportDialog.open()
    }

    Shortcut {
        sequence: "Meta+Shift+S"
        enabled: Qt.platform.os === "osx" && app.editorShortcutsAllowed

        onActivated: exportDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+D"
        enabled: app.editorShortcutsAllowed

        onActivated: app.doc.duplicateSelected()
    }

    Shortcut {
        sequence: "Meta+D"
        enabled: Qt.platform.os === "osx" && app.editorShortcutsAllowed

        onActivated: app.doc.duplicateSelected()
    }

    FileDialog {
        id: exportDialog

        title: "Export Selah project"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Selah projects (*.churchdeck)"]
        defaultSuffix: "churchdeck"

        onAccepted: app.doc.saveProject(selectedFile)
    }

    FileDialog {
        id: openDialog

        title: "Open Selah project"
        nameFilters: ["Selah projects (*.churchdeck)"]

        onAccepted: {
            if (app.doc.openProject(selectedFile)) {
                app.editingPresentationId = "";
                app.customPresentationTitle = "";
                app.editingTemplateId = "";
                app.programEditingItem = "";
                app.templateMode = false;
                app.switchWorkspace("design");
            }
        }
    }

    FileDialog {
        id: mediaDialog

        title: "Import image or video"
        nameFilters:
            ["Images and videos (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.svg *.mp4 *.mov *.m4v *.webm *.mkv)",
            "All files (*)"]

        onAccepted: {
            workspace.focusCanvas();
            app.doc.importMedia(selectedFile, !app.libraryImportOnly);
        }
    }

    ColorDialog {
        id: textColor

        title: "Text color"
        selectedColor: app.selectedValue("color", "#FFFFFF")

        onAccepted: app.update("color", selectedColor.toString())
    }

    ColorDialog {
        id: backgroundColor

        title: "Background tint"
        selectedColor: app.doc.backgroundColor

        onAccepted: app.doc.setBackground(selectedColor.toString())
    }

    Item {
        id: workspaceRoot

        anchors.fill: parent

        LibraryWorkspaceBar {
            id: libraryBar

            host: app
            width: parent.width
            visible: app.activeWorkspace === "libraries"
        }

        Item {
            id: librarySurface

            anchors.top: libraryBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: app.activeWorkspace === "libraries"
        }

        DesignWorkspaceBar {
            id: designBar

            host: app
            width: parent.width
            visible: app.activeWorkspace === "design"
        }

        Item {
            anchors.top: designBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: app.activeWorkspace === "design"

            EditorWorkspace {
                id: workspace

                host: app
                anchors.fill: parent
            }
        }

        PresentWorkspace {
            host: app
            anchors.fill: parent
            visible: app.activeWorkspace === "present"
        }
    }

    CustomSlidesPanel {
        id: customSlidesPanel

        workspaceMode: app.activeWorkspace === "libraries" && app.librarySection === "custom"
        parent: workspaceMode ? librarySurface : Overlay.overlay
        library: app.presentations

        onEditRequested: definition => app.editCustomPresentation(definition)
        onCreateRequested: {
            savePresentationDialog.createBlank = true;
            savePresentationDialog.open();
        }
    }

    SavePresentationDialog {
        id: savePresentationDialog

        parent: Overlay.overlay
        host: app
    }

    TemplateLibraryPanel {
        id: templatePanel

        objectName: "templatePanel"
        workspaceMode: app.activeWorkspace === "libraries" && app.librarySection === "templates"
        parent: workspaceMode ? librarySurface : Overlay.overlay
        library: app.templates

        onEditRequested: definition => app.editTemplate(definition)
    }

    SaveTemplateDialog {
        id: saveTemplateDialog

        parent: Overlay.overlay
        library: app.templates
        document: app.doc
        existingId: app.editingTemplateId

        onSaved: templateId => {
            app.editingTemplateId = templateId;
            app.notify("Template saved. Reapply it to update a prepared service.");
        }
    }

    ProgramPanel {
        id: programPanel

        objectName: "programPanel"
        workspaceMode: app.activeWorkspace === "programs"
        parent: workspaceMode ? workspaceRoot : Overlay.overlay
        store: app.programs
        controller: app.outputs
        document: app.doc
        operatorWindow: app
        editingItemId: app.programEditingItem
        keyboardAllowed: !songPanel.opened && !biblePanel.opened && !libraryPanel.opened &&
                         !outputPanel.opened && !templatePanel.opened && !saveTemplateDialog.opened
                         && !remotePanel.opened && !customSlidesPanel.opened &&
                         !savePresentationDialog.opened

        onPresentRequested: app.switchWorkspace("present")
        onSongsRequested: {
            songPanel.programTarget = app.programs;
            songPanel.open();
        }
        onBibleRequested: {
            biblePanel.programTarget = app.programs;
            biblePanel.open();
        }
        onMediaRequested: {
            libraryPanel.programTarget = app.programs;
            libraryPanel.open();
        }
        onOutputsRequested: outputPanel.open()
        onTemplatesRequested: app.showTemplates(app.programs)
        onEditRequested: (slides, itemId) => {
            app.editingPresentationId = "";
            app.customPresentationTitle = "";
            app.editingTemplateId = "";
            app.templateMode = false;
            if (app.doc.replaceSlides(slides)) {
                app.programEditingItem = itemId;
                app.switchWorkspace("design");
                app.notify("Editing a program item. Use Update program item when ready.");
            }
        }
    }

    SongLibraryPanel {
        id: songPanel

        objectName: "songsPanel"
        workspaceMode: app.activeWorkspace === "libraries" && app.librarySection === "songs"
        parent: workspaceMode ? librarySurface : Overlay.overlay
        library: app.content
        document: app.doc
        templates: app.templates

        onContentAdded: toProgram => app.contentAdded(toProgram, workspaceMode)
    }

    BibleLibraryPanel {
        id: biblePanel

        objectName: "biblePanel"
        workspaceMode: app.activeWorkspace === "libraries" && app.librarySection === "bible"
        parent: workspaceMode ? librarySurface : Overlay.overlay
        library: app.content
        document: app.doc
        templates: app.templates

        onContentAdded: toProgram => app.contentAdded(toProgram, workspaceMode)
    }

    MediaLibraryPanel {
        id: libraryPanel

        workspaceMode: app.activeWorkspace === "libraries" && app.librarySection === "media"
        parent: workspaceMode ? librarySurface : Overlay.overlay
        library: app.library
        document: app.doc

        onContentAdded: toProgram => app.contentAdded(toProgram, workspaceMode)
        onImportRequested: {
            app.libraryImportOnly = true;
            mediaDialog.open();
        }
    }

    OutputPanel {
        id: outputPanel

        parent: Overlay.overlay
        controller: app.outputs
        document: app.doc

        onRemoteRequested: {
            outputPanel.close();
            remotePanel.open();
        }
        onBroadcastRequested: app.showNdi()
    }

    RemotePanel {
        id: remotePanel

        objectName: "remotePanel"
        parent: Overlay.overlay
        service: app.remote
    }

    NdiPanel {
        id: ndiPanel

        parent: Overlay.overlay
        sender: app.ndi
    }

    PresentationOutput {
        id: audienceOutput

        controller: app.outputs
        role: "audience"
    }

    PresentationOutput {
        id: stageOutput

        controller: app.outputs
        role: "stage"
    }

    Window {
        id: outputWindow

        property var contentSnapshot: []
        property color slideColor: "#54435F"
        property real backgroundOpacity: 0.85
        property bool playing: false
        property bool audioEnabled: false

        width: 960
        height: 540
        title: "Selah — Output preview"
        color: "#000000"

        onVisibleChanged: if (!visible)
                              playing = false

        Rectangle {
            id: previewToolbar

            width: parent.width
            height: 44
            color: "#191919"

            Text {
                x: 14
                anchors.verticalCenter: parent.verticalCenter
                text: "OUTPUT PREVIEW"
                color: "#999999"
                font.pixelSize: 10
                font.letterSpacing: 1
            }

            Row {
                spacing: 8

                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    rightMargin: 10
                }

                StudioButton {
                    text: outputWindow.playing ? "Pause" : "Play"

                    onClicked: outputWindow.playing = !outputWindow.playing
                }

                StudioButton {
                    text: outputWindow.audioEnabled ? "Audio on" : "Audio off"
                    checked: outputWindow.audioEnabled

                    onClicked: outputWindow.audioEnabled = !outputWindow.audioEnabled
                }

                StudioButton {
                    text: "Close"

                    onClicked: outputWindow.close()
                }
            }
        }

        Item {
            anchors {
                top: previewToolbar.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            SlideRenderer {
                id: outputRenderer

                elements: outputWindow.contentSnapshot
                backgroundColor: outputWindow.slideColor
                ambientOpacity: outputWindow.backgroundOpacity
                playing: outputWindow.playing && outputWindow.visible
                audible: outputWindow.audioEnabled && outputWindow.visible
                showErrors: false
                scale: Math.min(parent.width / 1920, parent.height / 1080)
                transformOrigin: Item.TopLeft
                x: (parent.width - width * scale) / 2
                y: (parent.height - height * scale) / 2
            }
        }

        Shortcut {
            sequence: "Escape"

            onActivated: outputWindow.close()
        }
    }
}
