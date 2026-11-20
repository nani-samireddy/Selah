pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: panel

    required property var library
    required property var templates
    required property var document
    property var programTarget: null
    property var results: []
    property var draft: ({})
    property var sections: []
    property int sectionIndex: -1
    property var drafts: ({})
    property var arrangements: []
    property int arrangementIndex: 0
    property string message: ""
    property bool dirty: false
    property bool loading: false

    function refresh() {
        results = library.searchSongs(search.text);
    }

    function commitSection() {
        if (sectionIndex < 0 || sectionIndex >= sections.length)
            return;
        let list = sections.slice();
        let row = Object.assign({}, list[sectionIndex]);
        row.label = sectionName.text;
        row.primary = primary.text;
        row.secondary = secondary.text;
        list[sectionIndex] = row;
        sections = list;
    }

    function commitArrangement() {
        if (arrangementIndex < 0 || arrangementIndex >= arrangements.length)
            return;
        let list = arrangements.slice();
        list[arrangementIndex] = {
            name: list[arrangementIndex].name,
            order: arrangement.text.trim().split(/[\s,]+/).filter(s => s.length > 0)
        };
        arrangements = list;
    }

    function selectArrangement(index) {
        commitArrangement();
        arrangementIndex = index;
        arrangement.text = (arrangements[index].order || []).join(" ");
        dirty = true;
    }

    function gather() {
        commitSection();
        let song = Object.assign({}, draft);
        song.title = titleField.text;
        song.primaryLanguage = language.text;
        song.secondaryLanguage = translation.text;
        song.credits = credits.text;
        song.tags = tags.text;
        song.sections = sections;
        commitArrangement();
        song.arrangements = arrangements;
        song.activeArrangement = arrangementIndex;
        song.arrangement = arrangements[arrangementIndex].order;
        return song;
    }

    function selectSection(index) {
        commitSection();
        loading = true;
        sectionIndex = index;
        let row = sections[index] || {};
        sectionName.text = row.label || "";
        primary.text = row.primary || "";
        secondary.text = row.secondary || "";
        loading = false;
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
        language.text = draft.primaryLanguage || "English";
        translation.text = draft.secondaryLanguage || "";
        credits.text = draft.credits || "";
        tags.text = draft.tags || "";
        arrangements = draft.arrangements || [
                    {
                        name: "Default",
                        order: draft.arrangement || []
                    }
                ];
        arrangementIndex = draft.activeArrangement || 0;
        arrangement.text = (arrangements[arrangementIndex].order || []).join(" ");
        dirty = !!drafts[song.id || "new"];
        loading = false;
        selectSection(sections.length ? 0 : -1);
        message = "";
    }

    function create() {
        load({
                 title: "",
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
        commitSection();
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
        arrangement.text = (arrangement.text.trim() + " " + row.key).trim();
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

    width: Math.min(1080, parent.width - 40)
    height: Math.min(740, parent.height - 40)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    padding: 22

    background: Rectangle {
        color: "#191919"
        border.color: "#3B3B3B"
        radius: 12
    }

    onOpened: {
        refresh();
        if (sectionIndex < 0)
            create();
    }

    Connections {
        function onChanged() {
            panel.refresh();
        }

        target: panel.library
    }

    FileDialog {
        id: importDialog

        title: "Import Church Deck song"
        nameFilters: ["Church Deck songs (*.json)"]

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
        nameFilters: ["Church Deck songs (*.json)"]

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
            color: "#252525"
            border.color: "#444444"
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
                    text: "Songs"
                    color: "#EEEEEE"
                    font.pixelSize: 21
                    font.weight: Font.Medium
                }

                Text {
                    text: "Your lyrics. Every language. Ready for worship."
                    color: "#929292"
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Import song"

                onClicked: importDialog.open()
            }

            StudioButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 22

            ColumnLayout {
                Layout.preferredWidth: 230
                Layout.fillHeight: true
                spacing: 10

                LibraryField {
                    id: search

                    Layout.fillWidth: true
                    placeholderText: "Search title or lyrics…"

                    onTextChanged: panel.refresh()
                }

                StudioButton {
                    text: "New song"
                    iconName: "plus"
                    Layout.fillWidth: true

                    onClicked: panel.create()
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

                        width: ListView.view.width
                        height: 66

                        background: Rectangle {
                            color: panel.draft.id === songItem.modelData.id ? "#323232" :
                                                                              songItem.hovered
                                                                              ? "#242424" :
                                                                                "transparent"
                            radius: 6
                        }
                        contentItem: Column {
                            spacing: 6

                            Text {
                                width: parent.width
                                text: songItem.modelData.title
                                color: "#DDDDDD"
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: [songItem.modelData.primaryLanguage,
                                    songItem.modelData.secondaryLanguage].filter(Boolean).join(
                                    " · ")
                                color: "#888888"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        onClicked: panel.load(modelData)
                    }

                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 20
                        visible: panel.results.length === 0
                        text: search.text ? "No matching songs" :
                                            "Create your first song\nor import your lyrics."
                        color: "#888888"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }

                Text {
                    text: panel.results.length + " songs · On this computer"
                    color: "#777777"
                    font.pixelSize: 10
                }
            }

            Rectangle {
                Layout.fillHeight: true
                implicitWidth: 1
                color: "#333333"
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                RowLayout {
                    LibraryField {
                        id: titleField

                        Layout.fillWidth: true
                        placeholderText: "Song title"
                        font.pixelSize: 16

                        onTextEdited: panel.changed()
                    }

                    Text {
                        text: panel.dirty ? "Unsaved" : "Saved"
                        color: "#929292"
                        font.pixelSize: 11
                    }

                    StudioButton {
                        text: "Save song"

                        onClicked: panel.save()
                    }
                }

                RowLayout {
                    LibraryField {
                        id: language

                        Layout.fillWidth: true
                        placeholderText: "Primary language"

                        onTextEdited: panel.changed()
                    }

                    LibraryField {
                        id: translation

                        Layout.fillWidth: true
                        placeholderText: "Second language (optional)"

                        onTextEdited: panel.changed()
                    }
                }

                RowLayout {
                    LibraryField {
                        id: credits

                        Layout.fillWidth: true
                        placeholderText: "Author / copyright / licence"

                        onTextEdited: panel.changed()
                    }

                    LibraryField {
                        id: tags

                        Layout.preferredWidth: 170
                        placeholderText: "Tags"

                        onTextEdited: panel.changed()
                    }
                }

                RowLayout {
                    LibraryCombo {
                        model: panel.sections
                        textRole: "label"
                        currentIndex: panel.sectionIndex
                        Layout.fillWidth: true

                        onActivated: index => panel.selectSection(index)
                    }

                    StudioButton {
                        text: "+ Verse"

                        onClicked: panel.addSection("v")
                    }

                    StudioButton {
                        text: "+ Chorus"

                        onClicked: panel.addSection("c")
                    }
                }

                RowLayout {
                    Text {
                        text: panel.sectionIndex >= 0
                              ? panel.sections[panel.sectionIndex].key.toUpperCase() : ""
                        font.pixelSize: 11
                        color: "#888888"
                        Layout.preferredWidth: 30
                    }

                    LibraryField {
                        id: sectionName

                        Layout.fillWidth: true
                        placeholderText: "Section name"

                        onTextEdited: panel.changed()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true

                        Text {
                            text: "PRIMARY LYRICS"
                            color: "#888888"
                            font.pixelSize: 10
                            font.letterSpacing: 1
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: primary

                                objectName: "primaryLyrics"
                                color: "#DDDDDD"
                                placeholderTextColor: "#666666"
                                placeholderText: "One lyric line per row…"
                                font.pixelSize: 17
                                font.family: /[\u0C00-\u0C7F]/.test(text) ? "Noto Sans Telugu" :
                                                                            /[\u0900-\u097F]/.test(
                                                                                text) ? "Noto Sans Devanagari" :
                                                                                        "Arial"
                                padding: 12
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap

                                background: Rectangle {
                                    color: "#222222"
                                    border.color: primary.activeFocus ? "#777777" : "#363636"
                                    radius: 6
                                }

                                onTextChanged: panel.changed()
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true

                        Text {
                            text: "TRANSLATED LYRICS"
                            color: "#888888"
                            font.pixelSize: 10
                            font.letterSpacing: 1
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: secondary

                                objectName: "secondaryLyrics"
                                color: "#DDDDDD"
                                placeholderTextColor: "#666666"
                                placeholderText: "Matching translation on each row…"
                                font.pixelSize: 17
                                font.family: /[\u0C00-\u0C7F]/.test(text) ? "Noto Sans Telugu" :
                                                                            /[\u0900-\u097F]/.test(
                                                                                text) ? "Noto Sans Devanagari" :
                                                                                        "Arial"
                                padding: 12
                                selectByMouse: true
                                wrapMode: TextEdit.Wrap

                                background: Rectangle {
                                    color: "#222222"
                                    border.color: secondary.activeFocus ? "#777777" : "#363636"
                                    radius: 6
                                }

                                onTextChanged: panel.changed()
                            }
                        }
                    }
                }

                Text {
                    text: "Keep paired languages on matching rows. A blank row starts a new slide."
                    color: "#888888"
                    font.pixelSize: 11
                }

                RowLayout {
                    Text {
                        text: "Arrangement"
                        color: "#AAAAAA"
                        font.pixelSize: 12
                    }

                    LibraryField {
                        id: arrangement

                        Layout.fillWidth: true
                        placeholderText: "v1 c1 v2 c1"

                        onTextEdited: panel.changed()
                    }
                }

                RowLayout {
                    LibraryCombo {
                        model: panel.arrangements
                        textRole: "name"
                        currentIndex: panel.arrangementIndex
                        Layout.preferredWidth: 180

                        onActivated: index => panel.selectArrangement(index)
                    }

                    StudioButton {
                        text: "+ Arrangement"
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

                Text {
                    text: "Use section keys in order; repeat a key to repeat its lyrics."
                    color: "#777777"
                    font.pixelSize: 11
                }
            }
        }

        RowLayout {
            Text {
                text: "Layout"
                color: "#999999"
                font.pixelSize: 11
            }

            TemplateSelector {
                id: templateChoice

                library: panel.templates
                kind: panel.sections.some((section, index) => index === panel.sectionIndex ? !
                                                                                             !secondary.text.trim(
                                                                                                 ) : !!section.secondary
                                                                                             ?.trim())
                      ? "bilingual" : "song"
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#333333"
        }

        RowLayout {
            Text {
                Layout.fillWidth: true
                text: panel.message
                color: "#AAAAAA"
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }

            StudioButton {
                text: "Export"
                enabled: !!panel.draft.id && !panel.dirty

                onClicked: exportDialog.open()
            }

            Text {
                text: "Lines / slide"
                color: "#999999"
                font.pixelSize: 11
            }

            SpinBox {
                id: lineCount

                from: 1
                to: 6
                value: 3
                implicitWidth: 100
                palette.text: "#DDDDDD"
                palette.button: "#303030"
                palette.base: "#242424"
            }

            StudioButton {
                text: panel.programTarget ? "Add to program" : "Add slides"
                iconName: "plus"
                accented: true

                onClicked: {
                    if (panel.save()) {
                        let target = panel.programTarget || panel.document;
                        if (target.addSongSlides ? target.addSongSlides(panel.library.song(
                                                                            panel.draft.id),
                                                                        lineCount.value,
                                                                        templateChoice.definition) :
                                                   target.addSong(panel.library.song(panel.draft.id),
                                                                  lineCount.value,
                                                                  templateChoice.definition))
                            panel.close();
                        else
                            panel.message = target.status;
                    }
                }
            }
        }
    }
}
