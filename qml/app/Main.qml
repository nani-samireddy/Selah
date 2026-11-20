pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: app

    required property var doc
    required property var library
    required property var content
    required property var programs
    property string programEditingItem: ""
    required property var outputs
    required property var templates
    property string editingTemplateId: ""
    required property var remote
    required property var ndi
    property var element: app.doc.selectedElement
    property bool templateMode: false
    property string editingId: ""
    property bool videosPlaying: false
    property bool snap: true
    property bool handTool: false
    property real zoom: 1
    property real panX: 0
    property real panY: 0
    property string toast: ""
    property bool libraryImportOnly: false
    readonly property bool editorShortcutsAllowed: !programPanel.opened && !templatePanel.opened &&
                                                   !saveTemplateDialog.opened && !songPanel.opened
                                                   && !biblePanel.opened && !libraryPanel.opened &&
                                                   !outputPanel.opened && !ndiPanel.opened &&
                                                   !remotePanel.opened

    function update(key, value) {
        if (app.doc.selected >= 0) {
            let patch = {};
            patch[key] = value;
            app.doc.updateElement(app.doc.selected, patch);
        }
    }

    function selectedValue(key, fallback) {
        return element[key] === undefined ? fallback : element[key];
    }

    function rendererProbe() {
        return workspace.rendererProbe();
    }

    function showSongs() {
        libraryPanel.close();
        songPanel.programTarget = null;
        songPanel.open();
    }

    function showBible() {
        libraryPanel.close();
        biblePanel.programTarget = null;
        biblePanel.open();
    }

    function showLibrary() {
        outputPanel.close();
        libraryPanel.programTarget = null;
        libraryPanel.open();
    }

    function showTemplates(target) {
        templatePanel.programTarget = target || null;
        templatePanel.open();
    }

    function editTemplate(definition) {
        if (app.doc.replaceSlides([definition.slide])) {
            programPanel.close();
            app.editingTemplateId = definition.id;
            app.programEditingItem = "";
            app.templateMode = true;
        }
    }

    function saveCurrentTemplate() {
        saveTemplateDialog.open();
    }

    function showPrograms() {
        libraryPanel.close();
        songPanel.close();
        biblePanel.close();
        outputPanel.close();
        programPanel.open();
    }

    function showOutputs() {
        libraryPanel.close();
        outputPanel.open();
    }

    function showNdi() {
        libraryPanel.close();
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
    visible: true
    title: "Church Deck — Sunday Worship"
    color: "#111111"
    palette.window: "#171717"
    palette.text: "#DDDDDD"
    palette.base: "#252525"
    palette.button: "#252525"
    palette.buttonText: "#DDDDDD"
    font.family: "Arial"

    header: Rectangle {
        height: 62
        color: "#191919"

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: "#303030"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: app.width > 1300 ? 16 : 9

            Image {
                source: "../assets/icons/aperture.svg"
                sourceSize.width: 48
                sourceSize.height: 48
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }

            Text {
                text: "church deck"
                color: "#F2F2F2"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: -0.4
            }

            Rectangle {
                implicitWidth: 1
                implicitHeight: 18
                color: "#3A3A3A"
                Layout.leftMargin: 4
                Layout.rightMargin: 4
            }

            Column {
                spacing: 3
                visible: app.width > 1250

                Text {
                    text: app.programs.title || "Presentation editor"
                    elide: Text.ElideRight
                    width: 150
                    color: "#DADADA"
                    font.pixelSize: 12
                }

                Text {
                    text: app.programs.id ? (app.programs.date || "Service program") :
                                            "Local presentation"
                    color: "#777777"
                    font.pixelSize: 10
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: app.doc.importing ? "Importing media…" : app.doc.status
                color: "#858585"
                font.pixelSize: 11
                visible: app.width > 1300
            }

            StudioButton {
                text: "Programs"

                onClicked: app.showPrograms()
            }

            StudioButton {
                text: "Libraries"
                iconName: "layers"

                onClicked: librariesMenu.popup()
            }

            Menu {
                id: librariesMenu

                MenuItem {
                    text: "Songs"

                    onTriggered: app.showSongs()
                }

                MenuItem {
                    text: "Bible"

                    onTriggered: app.showBible()
                }

                MenuItem {
                    text: "Templates"

                    onTriggered: app.showTemplates(null)
                }

                MenuItem {
                    text: "Media"

                    onTriggered: app.showLibrary()
                }
            }

            StudioButton {
                text: "Outputs"

                onClicked: outputPanel.open()
            }

            StudioButton {
                text: "Open"
                quiet: true

                onClicked: openDialog.open()
            }

            StudioButton {
                text: "Export"
                iconName: "upload"

                onClicked: exportDialog.open()
            }

            StudioButton {
                text: "Preview"
                iconName: "play"
                accented: true
                hint: "Open a clean output preview"

                onClicked: app.preview()
            }

            StudioButton {
                text: "Go live"
                hint: "Take the selected slide live"

                onClicked: {
                    workspace.forceActiveFocus();
                    app.outputs.takeLive(app.doc.slides, app.doc.currentSlide);
                    app.notify("Live: " + app.outputs.liveTitle);
                }
            }
        }
    }
    footer: Rectangle {
        height: 30
        color: "#171717"

        Rectangle {
            width: parent.width
            height: 1
            color: "#2A2A2A"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            Text {
                text: app.templateMode ? "Template editor  /  Reusable layout" :
                                         "Design mode  /  Changes stay in draft"
                color: "#777777"
                font.pixelSize: 10
                Layout.fillWidth: true
            }

            Text {
                text: app.outputs.liveIndex >= 0 ? "LIVE  /  " + app.outputs.liveTitle + "  ·  "
                                                   + app.outputs.status : "Outputs stopped"
                color: "#929292"
                font.pixelSize: 10
            }
        }
    }

    Component.onCompleted: app.outputs.bindWindows(audienceOutput, stageOutput, app)
    onClosing: {
        programPanel.flushItem();
        app.programs.save();
        workspace.forceActiveFocus();
        app.doc.saveProject();
        app.outputs.stop("audience");
        app.outputs.stop("stage");
        app.ndi.stop();
        app.remote.stop();
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
                app.doc.saveProject();
                app.notify("Project saved on this computer");
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

        title: "Export Church Deck project"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Church Deck projects (*.churchdeck)"]
        defaultSuffix: "churchdeck"

        onAccepted: app.doc.saveProject(selectedFile)
    }

    FileDialog {
        id: openDialog

        title: "Open Church Deck project"
        nameFilters: ["Church Deck projects (*.churchdeck)"]

        onAccepted: {
            if (app.doc.openProject(selectedFile)) {
                app.editingTemplateId = "";
                app.programEditingItem = "";
                app.templateMode = false;
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

    EditorWorkspace {
        id: workspace

        host: app
        anchors.fill: parent
    }

    TemplateLibraryPanel {
        id: templatePanel

        objectName: "templatePanel"
        parent: Overlay.overlay
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
        parent: Overlay.overlay
        store: app.programs
        controller: app.outputs
        document: app.doc
        operatorWindow: app
        editingItemId: app.programEditingItem
        keyboardAllowed: !songPanel.opened && !biblePanel.opened && !libraryPanel.opened &&
                         !outputPanel.opened && !templatePanel.opened && !saveTemplateDialog.opened
                         && !remotePanel.opened

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
            app.editingTemplateId = "";
            app.templateMode = false;
            if (app.doc.replaceSlides(slides)) {
                app.programEditingItem = itemId;
                app.notify("Edit this item, then Programs → Update from editor");
            }
        }
    }

    SongLibraryPanel {
        id: songPanel

        objectName: "songsPanel"
        parent: Overlay.overlay
        library: app.content
        document: app.doc
        templates: app.templates
    }

    BibleLibraryPanel {
        id: biblePanel

        objectName: "biblePanel"
        parent: Overlay.overlay
        library: app.content
        document: app.doc
        templates: app.templates
    }

    MediaLibraryPanel {
        id: libraryPanel

        parent: Overlay.overlay
        library: app.library
        document: app.doc

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
        title: "Church Deck — Output preview"
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
