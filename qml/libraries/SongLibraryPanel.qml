pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

Popup {
    id: panel

    property bool workspaceMode: false
    readonly property bool quickSongOpen: quickSong.opened
    required property var library
    required property var templates
    required property var document
    property var programTarget: null
    property var results: []
    property string selectedTag: ""
    readonly property var tagOptions: [
        {
            text: "All tags",
            value: ""
        }
    ].concat(library.songTags.map(tag => ({
        text: tag,
        value: tag
    })))
    property var draft: ({})
    property var sections: []
    property int sectionIndex: -1
    property var drafts: ({})
    property var arrangements: []
    property int arrangementIndex: 0
    property string message: ""
    property bool dirty: false
    property bool loading: false
    property bool detailsOpen: false
    property var sectionOrder: []

    signal contentAdded(bool toProgram)

    function rememberLanguages() {
        songPreferences.primaryLanguage = language.value;
        songPreferences.secondaryLanguage = translation.value;
        let recent = [language.value, translation.value].concat(songPreferences.recentLanguages);
        songPreferences.recentLanguages = recent.filter((name, index) => name && recent.indexOf(
                                                                             name) === index).slice(
                    0, 6);
        changed();
    }

    function addToTarget(programId = "") {
        if (!save())
            return;
        const target = programTarget || document;
        const song = library.song(draft.id);
        const ok = programTarget ? target.addSong(song, lineCount.value, templateChoice.definition,
                                                  programId) : target.addSongSlides(song,
                                                                                    lineCount.value,
                                                                                    templateChoice.definition);
        if (ok && (!programTarget || !programId || programId === programTarget.id))
            finishAddition();
        else
            message = target.status;
    }

    function finishAddition() {
        let toProgram = !!programTarget;
        close();
        contentAdded(toProgram);
    }

    function lyricFont(text) {
        if (/[\u0C00-\u0C7F]/.test(text))
            return "Noto Sans Telugu";
        if (/[\u0900-\u097F]/.test(text))
            return "Noto Sans Devanagari";
        return LibraryStyle.fontFamily;
    }

    function refresh() {
        if (selectedTag && !library.songTags.some(tag => tag.toLocaleLowerCase()
                                                         === selectedTag.toLocaleLowerCase()))
            selectedTag = "";
        results = library.searchSongs(search.text, selectedTag);
    }

    function commitArrangement() {
        if (arrangementIndex < 0 || arrangementIndex >= arrangements.length)
            return;
        let list = arrangements.slice();
        list[arrangementIndex] = {
            name: list[arrangementIndex].name,
            order: sectionOrder.slice()
        };
        arrangements = list;
    }

    function selectArrangement(index) {
        commitArrangement();
        arrangementIndex = index;
        sectionOrder = (arrangements[index].order || []).slice();
        dirty = true;
    }

    function gather() {
        let song = Object.assign({}, draft);
        song.title = titleField.text;
        song.primaryLanguage = language.value;
        song.secondaryLanguage = translation.value;
        song.credits = credits.text;
        tagEditor.commit();
        song.tags = tagEditor.tags.join(", ");
        song.sections = sections;
        commitArrangement();
        song.arrangements = arrangements;
        song.activeArrangement = arrangementIndex;
        song.arrangement = arrangements[arrangementIndex].order;
        return song;
    }

    function selectSection(index) {
        sectionIndex = index;
    }

    function focusSection(index) {
        selectSection(index);
        const card = sectionCards.sectionItem(index);
        if (!card)
            return;
        card.focusLyrics();
        const flick = songDetailsScroll.contentItem;
        const y = card.mapToItem(flick.contentItem, 0, 0).y;
        flick.contentY = Math.max(0, Math.min(y, flick.contentHeight - flick.height));
    }

    function load(song) {
        if (dirty) {
            let cache = Object.assign({}, drafts);
            cache[draft.id || "new"] = gather();
            drafts = cache;
        }
        loading = true;
        draft = JSON.parse(JSON.stringify(drafts[song.id || "new"] || song));
        sections = draft.sections || [];
        sectionIndex = -1;
        titleField.text = draft.title || "";
        language.value = draft.primaryLanguage || "English";
        translation.value = draft.secondaryLanguage || "";
        credits.text = draft.credits || "";
        tagEditor.reset(draft.tags || "");
        arrangements = draft.arrangements || [
                    {
                        name: "Default",
                        order: draft.arrangement || []
                    }
                ];
        arrangementIndex = draft.activeArrangement || 0;
        sectionOrder = (arrangements[arrangementIndex].order || []).slice();
        dirty = !!drafts[song.id || "new"];
        loading = false;
        selectSection(sections.length ? 0 : -1);
        message = "";
    }

    function create() {
        load({
                 title: "",
                 primaryLanguage: songPreferences.primaryLanguage,
                 secondaryLanguage: songPreferences.secondaryLanguage,
                 sections: [
                     {
                         key: "v1",
                         label: "Verse 1",
                         primary: "",
                         secondary: ""
                     }
                 ],
                 arrangement: ["v1"]
             });
    }

    function changed() {
        if (!loading)
            dirty = true;
    }

    function addSection(kind) {
        let count = 1;
        while (sections.some(s => s.key === kind + count))
            count++;
        let row = {
            key: kind + count,
            label: (kind === "v" ? "Verse " : "Chorus ") + count,
            primary: "",
            secondary: ""
        };
        sections = sections.concat([row]);
        sectionOrder = sectionOrder.concat([row.key]);
        selectSection(sections.length - 1);
        dirty = true;
    }

    function save() {
        let song = gather();
        let id = library.saveSong(song);
        message = library.status;
        if (!id)
            return false;
        let cache = Object.assign({}, drafts);
        delete cache[draft.id || "new"];
        drafts = cache;
        draft = library.song(id);
        dirty = false;
        refresh();
        return true;
    }

    closePolicy: workspaceMode ? Popup.NoAutoClose : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: workspaceMode ? parent.width : Math.min(1080, parent.width - 40)
    height: workspaceMode ? parent.height : Math.min(740, parent.height - 40)
    x: workspaceMode ? 0 : (parent.width - width) / 2
    y: workspaceMode ? 0 : (parent.height - height) / 2
    modal: !workspaceMode
    focus: true
    padding: LibraryStyle.pagePadding

    background: Rectangle {
        color: LibraryStyle.panel
        border.width: panel.workspaceMode ? 0 : 1
        border.color: LibraryStyle.tone("#3B3B3B")
        radius: panel.workspaceMode ? 0 : 12
    }

    onOpened: {
        refresh();
        if (sectionIndex < 0) {
            if (results.length > 0)
                load(library.song(results[0].id));
            else
                create();
        }
    }

    Settings {
        id: songPreferences

        property string primaryLanguage: "English"
        property string secondaryLanguage: ""
        property var recentLanguages: []

        category: "SongLanguages"
    }

    Connections {
        function onChanged() {
            panel.refresh();
        }

        target: panel.library
    }

    QuickSongDialog {
        id: quickSong

        parent: Overlay.overlay
        library: panel.library
        primaryLanguage: songPreferences.primaryLanguage

        onSongCreated: songId => {
            panel.selectedTag = "";
            search.clear();
            panel.load(panel.library.song(songId));
            panel.refresh();
            panel.message = "Song created";
        }
    }

    FileDialog {
        id: importDialog

        title: "Import Selah song"
        nameFilters: ["Selah songs (*.json)"]

        onAccepted: {
            panel.library.importSong(selectedFile);
            panel.message = panel.library.status;
            panel.refresh();
        }
    }

    FileDialog {
        id: exportDialog

        title: "Export song"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Selah songs (*.json)"]

        onAccepted: {
            panel.library.exportSong(panel.draft.id, selectedFile);
            panel.message = panel.library.status;
        }
    }

    Dialog {
        id: arrangementDialog

        title: "New arrangement"
        modal: true
        anchors.centerIn: parent
        width: 340
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: LibraryStyle.tone("#252525")
            border.color: LibraryStyle.tone("#444444")
            radius: 8
        }
        contentItem: LibraryField {
            id: arrangementName

            placeholderText: "Name, e.g. Sunday or acoustic"
        }

        onAccepted: {
            let name = arrangementName.text.trim();
            if (!name || panel.arrangements.some(a => a.name === name)) {
                panel.message = "Enter a unique arrangement name";
                return;
            }
            panel.commitArrangement();
            panel.arrangements = panel.arrangements.concat([
                                                               {
                                                                   name: name,
                                                                   order: panel.arrangements[panel.arrangementIndex].order.slice(
                                                                       )
                                                               }
                                                           ]);
            panel.selectArrangement(panel.arrangements.length - 1);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Column {
                spacing: 5

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: "Songs"
                    color: LibraryStyle.tone("#EEEEEE")
                    font.pixelSize: 22
                    font.weight: Font.Medium
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: "Your lyrics. Every language. Ready for worship."
                    color: LibraryStyle.tone("#929292")
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Import song"

                onClicked: importDialog.open()
            }

            LibraryButton {
                text: "Close"
                visible: !panel.workspaceMode
                quiet: true

                onClicked: panel.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 24

            ColumnLayout {
                Layout.preferredWidth: 260
                Layout.minimumWidth: 240
                Layout.maximumWidth: 280
                Layout.fillHeight: true
                spacing: LibraryStyle.fieldGap

                LibraryField {
                    id: search

                    leadingIcon: "search"
                    Layout.fillWidth: true
                    placeholderText: "Search title, lyrics or tags…"

                    onTextChanged: panel.refresh()
                }

                LibraryButton {
                    text: "New song"
                    iconName: "plus"
                    Layout.fillWidth: true

                    onClicked: panel.create()
                }

                LibraryButton {
                    objectName: "quickSongAction"
                    text: "Quick Song"
                    iconName: "type"
                    Layout.fillWidth: true

                    onClicked: quickSong.open()
                }

                LibraryCombo {
                    id: tagFilter

                    objectName: "songTagFilter"
                    Layout.fillWidth: true
                    model: panel.tagOptions
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: Math.max(0, panel.tagOptions.findIndex(row => row.value
                                                                                === panel.selectedTag))
                    Accessible.name: "Filter songs by tag"

                    onActivated: {
                        panel.selectedTag = currentValue;
                        panel.refresh();
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 5
                    model: panel.results

                    ScrollBar.vertical: ScrollBar {
                    }
                    delegate: ItemDelegate {
                        id: songItem

                        required property var modelData

                        Accessible.name: songItem.modelData.title
                        width: ListView.view.width
                        height: panel.library.parseSongTags(modelData.tags || "").length ? 82 : 64
                        padding: 12

                        background: Rectangle {
                            border.width: 1
                            border.color: panel.draft.id === songItem.modelData.id
                                          ? LibraryStyle.tone("#505050") : "transparent"
                            color: panel.draft.id === songItem.modelData.id ? LibraryStyle.tone(
                                                                                  "#323232") :
                                                                              songItem.hovered
                                                                              ? LibraryStyle.tone(
                                                                                    "#242424") :
                                                                                "transparent"
                            radius: 6
                        }
                        contentItem: Column {
                            spacing: 6

                            Text {
                                font.family: LibraryStyle.fontFamily
                                width: parent.width
                                text: songItem.modelData.title
                                color: LibraryStyle.tone("#DDDDDD")
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                font.family: LibraryStyle.fontFamily
                                width: parent.width
                                text: [songItem.modelData.primaryLanguage,
                                    songItem.modelData.secondaryLanguage].filter(Boolean).join(
                                    " · ")
                                color: LibraryStyle.tone("#888888")
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: panel.library.parseSongTags(songItem.modelData.tags
                                                                  || "").join(" · ")
                                visible: text.length > 0
                                color: LibraryStyle.muted
                                font.family: LibraryStyle.fontFamily
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }

                        onClicked: panel.load(modelData)
                    }

                    Text {
                        font.family: LibraryStyle.fontFamily
                        anchors.centerIn: parent
                        width: parent.width - 20
                        visible: panel.results.length === 0
                        text: search.text || panel.selectedTag ? "No matching songs" :
                                                                 "Create your first song\nor import your lyrics."
                        color: LibraryStyle.tone("#888888")
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: panel.results.length + " songs · On this computer"
                    color: LibraryStyle.tone("#777777")
                    font.pixelSize: 10
                }
            }

            Rectangle {
                Layout.fillHeight: true
                implicitWidth: 1
                color: LibraryStyle.tone("#333333")
            }

            ScrollView {
                id: songDetailsScroll

                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: songDetailsScroll.availableWidth
                    spacing: LibraryStyle.fieldGap

                    RowLayout {
                        LibraryField {
                            id: titleField

                            Layout.fillWidth: true
                            placeholderText: "Song title"
                            font.pixelSize: 14

                            onTextEdited: panel.changed()
                        }

                        Text {
                            font.family: LibraryStyle.fontFamily
                            text: panel.dirty ? "Unsaved" : panel.draft.id ? "Saved" : "New song"
                            color: LibraryStyle.tone("#929292")
                            font.pixelSize: 11
                        }

                        LibraryButton {
                            text: "Save song"

                            onClicked: panel.save()
                        }
                    }

                    RowLayout {
                        SongLanguageCombo {
                            id: language

                            objectName: "songPrimaryLanguage"
                            Layout.fillWidth: true
                            value: "English"
                            recent: songPreferences.recentLanguages

                            onChosen: panel.rememberLanguages()
                        }

                        LibraryButton {
                            text: panel.detailsOpen ? "Hide details" : "Details"
                            quiet: true

                            onClicked: panel.detailsOpen = !panel.detailsOpen
                        }

                        SongLanguageCombo {
                            id: translation

                            objectName: "songSecondaryLanguage"
                            Layout.fillWidth: true
                            optional: true
                            value: ""
                            recent: songPreferences.recentLanguages

                            onChosen: panel.rememberLanguages()
                        }
                    }

                    RowLayout {
                        visible: panel.detailsOpen

                        LibraryField {
                            id: credits

                            Layout.fillWidth: true
                            placeholderText: "Author / copyright / licence"

                            onTextEdited: panel.changed()
                        }
                    }

                    SongTagEditor {
                        id: tagEditor

                        objectName: "songTagEditor"
                        Layout.fillWidth: true
                        library: panel.library

                        onEdited: panel.changed()
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 12

                        LibraryCaption {
                            text: "Sections & lyrics"
                            Layout.fillWidth: true
                        }

                        LibraryButton {
                            text: "Verse"
                            iconName: "plus"

                            onClicked: panel.addSection("v")
                        }

                        LibraryButton {
                            text: "Chorus"
                            iconName: "plus"

                            onClicked: panel.addSection("c")
                        }
                    }

                    SongSectionsEditor {
                        id: sectionCards

                        objectName: "songSectionsEditor"
                        Layout.fillWidth: true
                        sections: panel.sections
                        primaryLanguage: language.value
                        secondaryLanguage: translation.value
                        fontForText: text => panel.lyricFont(text)
                        showDetails: panel.detailsOpen

                        onEdited: sections => {
                            panel.sections = sections;
                            panel.changed();
                        }
                        onFocused: index => panel.sectionIndex = index
                    }

                    Text {
                        text: "All sections are shown above. Leave a blank line to start a new slide."
                        color: LibraryStyle.muted
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    SongOrderEditor {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        sections: panel.sections
                        order: panel.sectionOrder

                        onEdited: keys => {
                            panel.sectionOrder = keys;
                            panel.changed();
                        }
                        onEditSection: index => panel.focusSection(index)
                    }

                    RowLayout {
                        visible: panel.detailsOpen || panel.arrangements.length > 1

                        LibraryCombo {
                            model: panel.arrangements
                            textRole: "name"
                            currentIndex: panel.arrangementIndex
                            Layout.preferredWidth: 180

                            onActivated: index => panel.selectArrangement(index)
                        }

                        LibraryButton {
                            text: "New order"
                            iconName: "plus"
                            implicitHeight: 30

                            onClicked: {
                                arrangementName.text = "";
                                arrangementDialog.open();
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        RowLayout {
            Text {
                font.family: LibraryStyle.fontFamily
                text: "Layout"
                color: LibraryStyle.tone("#999999")
                font.pixelSize: 11
            }

            TemplateSelector {
                id: templateChoice

                library: panel.templates
                kind: panel.sections.some(section => !!section.secondary?.trim()) ? "bilingual" :
                                                                                    "song"
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: LibraryStyle.tone("#333333")
        }

        RowLayout {
            Text {
                font.family: LibraryStyle.fontFamily
                Layout.fillWidth: true
                text: panel.message
                color: LibraryStyle.tone("#AAAAAA")
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }

            LibraryButton {
                text: "Export"
                enabled: !!panel.draft.id && !panel.dirty

                onClicked: exportDialog.open()
            }

            Text {
                font.family: LibraryStyle.fontFamily
                text: "Lines / slide"
                color: LibraryStyle.tone("#999999")
                font.pixelSize: 11
            }

            LibrarySpinBox {
                id: lineCount

                from: 1
                to: 6
                value: 3
                implicitWidth: 100
                palette.text: LibraryStyle.tone("#DDDDDD")
                palette.button: LibraryStyle.tone("#303030")
                palette.base: LibraryStyle.tone("#242424")
            }

            ProgramAddButton {
                objectName: "songProgramDestination"
                visible: !!panel.programTarget
                store: panel.programTarget

                onAddRequested: programId => panel.addToTarget(programId)
            }

            LibraryButton {
                visible: !panel.programTarget
                text: "Add slides"
                iconName: "plus"
                accented: true

                onClicked: panel.addToTarget()
            }
        }
    }
}
