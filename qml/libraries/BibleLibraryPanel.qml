pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    property bool workspaceMode: false
    required property var library
    required property var templates
    required property var document
    property var programTarget: null
    property var versions: library.translations
    property var version: versions[versionPicker.currentIndex] || ({})
    property var bookList: library.books(version.id || "")
    property var book: bookList[bookPicker.currentIndex] || ({})
    property var chapterList: library.chapters(version.id || "", book.code || "")
    property int chapterIndex: 0
    property var chapter: chapterList[chapterIndex] || ({})
    property int highlightedVerse: -1
    property var chapterVerses: library.passage(version.id || "", book.code || "", chapter.chapter
                                                || 1, chapter.first ?? 1, chapter.last ?? 1)
    property var verses: []
    property var selected: []
    property string message: ""
    readonly property var secondaryVersions: [
        {
            id: "",
            title: "None · single Bible"
        }
    ].concat(versions.filter(record => record.id !== version.id))
    readonly property string secondaryId: (secondaryVersions[secondaryPicker.currentIndex]
                                           || {}).id || ""
    readonly property var pairedSelection: library.pairScripture(selected.length ? selected.map(i
                                                                                                => verses[i]) :
                                                                                   verses, secondaryId)
    readonly property string pairingError: pairedSelection.error || ""
    property bool searchMode: search.text.trim().length > 0

    signal contentAdded(bool toProgram)

    function addToTarget(programId = "") {
        if (pairingError) {
            message = pairingError;
            return;
        }
        const rows = pairedSelection.verses || [];
        const target = programTarget || document;
        const ok = programTarget ? target.addScripture(rows, lines.value, templateChoice.definition,
                                                       programId) : target.addScriptureSlides(rows,
                                                                                              lines.value,
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

    function refresh() {
        debounce.stop();
        selected = [];
        message = "";
        verses = searchMode ? library.searchVerses(version.id || "", search.text) : chapterVerses;
        highlightedVerse = -1;
        verseView.positionViewAtBeginning();
    }

    function schedule() {
        debounce.restart();
    }

    function choose(index) {
        let rows = selected.slice();
        let at = rows.indexOf(index);
        if (at >= 0)
            rows.splice(at, 1);
        else
            rows.push(index);
        rows.sort((a, b) => a - b);
        selected = rows;
    }

    function jumpToVerse(number) {
        if (searchMode) {
            search.text = "";
            debounce.stop();
            refresh();
        }
        debounce.stop();
        const index = verses.findIndex(verse => verse.verse === number);
        if (index < 0)
            return;
        highlightedVerse = number;
        selected = [index];
        verseView.forceLayout();
        verseView.currentIndex = index;
        verseView.positionViewAtIndex(index, ListView.Center);
        Qt.callLater(() => {
            if (panel.highlightedVerse !== number)
                return;
            debounce.stop();
            verseView.forceLayout();
            verseView.currentIndex = index;
            verseView.positionViewAtIndex(index, ListView.Center);
        });
    }

    function openPassage(translation, code, chapterNumber, first, last) {
        search.text = "";
        for (let i = 0; i < versions.length; i++)
            if (versions[i].id === translation)
                versionPicker.currentIndex = i;
        for (let i = 0; i < bookList.length; i++)
            if (bookList[i].code === code)
                bookPicker.currentIndex = i;
        for (let i = 0; i < chapterList.length; i++)
            if (chapterList[i].chapter === chapterNumber)
                chapterIndex = i;
        Qt.callLater(function () {
            debounce.stop();
            panel.refresh();
            panel.jumpToVerse(first);
            panel.selected = panel.verses.map((verse, index) => verse.verse >= first && verse.verse
                                                                <= last ? index : -1).filter(index
                                                                                             => index
                                                                                                >= 0);
        });
    }

    closePolicy: workspaceMode ? Popup.NoAutoClose : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: workspaceMode ? parent.width : Math.min(1000, parent.width - 40)
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

    onOpened: refresh()

    Connections {
        function onBibleImported(translationId) {
            for (let i = 0; i < panel.versions.length; ++i)
                if (panel.versions[i].id === translationId)
                    versionPicker.currentIndex = i;
            bookPicker.currentIndex = 0;
            panel.chapterIndex = 0;
            search.text = "";
            Qt.callLater(panel.refresh);
        }

        function onChanged() {
            if (panel.opened)
                panel.schedule();
        }

        target: panel.library
    }

    Timer {
        id: debounce

        interval: 230

        onTriggered: panel.refresh()
    }

    BibleImportDialog {
        id: importDialog

        parent: Overlay.overlay
        library: panel.library
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Column {
                spacing: 5

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: "Bible"
                    color: LibraryStyle.tone("#EEEEEE")
                    font.pixelSize: 22
                    font.weight: Font.Medium
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: "Find a passage. Bring the Word to the screen."
                    color: LibraryStyle.tone("#929292")
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: panel.library.busy ? (panel.library.bibleImportPhase === "downloading"
                                            ? "Downloading…" : "Importing…") : "Import translation"
                enabled: !panel.library.busy

                onClicked: importDialog.open()
            }

            LibraryButton {
                text: "Close"
                visible: !panel.workspaceMode
                quiet: true

                onClicked: panel.close()
            }
        }

        BibleImportProgress {
            library: panel.library
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 24

            ColumnLayout {
                id: scriptureSidebar

                Layout.preferredWidth: 250
                Layout.minimumWidth: 220
                Layout.maximumWidth: 260
                Layout.fillHeight: true
                spacing: 12

                LibraryCombo {
                    id: versionPicker

                    objectName: "bibleVersionPicker"
                    Layout.fillWidth: true
                    model: panel.versions
                    textRole: "abbreviation"
                    Accessible.name: "Bible translation"

                    onActivated: {
                        secondaryPicker.currentIndex = 0;
                        bookPicker.currentIndex = 0;
                        panel.chapterIndex = 0;
                        search.clear();
                        panel.refresh();
                    }
                }

                LibraryCaption {
                    text: "Second Bible (optional)"
                }

                LibraryCombo {
                    id: secondaryPicker

                    objectName: "bibleSecondaryPicker"
                    Layout.fillWidth: true
                    model: panel.secondaryVersions
                    textRole: "title"
                    Accessible.name: "Second Bible translation"

                    onActivated: {
                        panel.message = "";
                        templateChoice.selectedId = "";
                    }
                }

                LibraryCombo {
                    id: bookPicker

                    objectName: "bibleBookPicker"
                    Layout.fillWidth: true
                    model: panel.bookList
                    textRole: "name"
                    Accessible.name: "Bible book"

                    onActivated: {
                        panel.chapterIndex = 0;
                        search.clear();
                        panel.refresh();
                    }
                }

                LibraryCaption {
                    text: "Chapters · " + panel.chapterList.length
                }

                ScriptureNumberGrid {
                    objectName: "bibleChapterGrid"
                    Layout.fillWidth: true
                    Layout.minimumHeight: 32
                    Layout.maximumHeight: Math.max(32, (panel.availableHeight - 250) * 0.4)
                    Layout.preferredHeight: Math.min(217, Math.ceil(panel.chapterList.length / 6) * 37,
                                                     Math.max(32, (panel.availableHeight - 250)
                                                              * 0.4))
                    numbers: panel.chapterList.map(chapter => chapter.chapter)
                    selectedNumber: panel.chapter.chapter || 1
                    accessiblePrefix: "Chapter"

                    onChosen: number => {
                        panel.chapterIndex = panel.chapterList.findIndex(chapter => chapter.chapter
                                                                                    === number);
                        search.clear();
                        panel.refresh();
                    }
                }

                LibraryCaption {
                    text: "Verses · " + panel.chapterVerses.length
                }

                ScriptureNumberGrid {
                    objectName: "bibleVerseGrid"
                    Layout.minimumHeight: 69
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    numbers: panel.chapterVerses.map(verse => verse.verse)
                    selectedNumber: panel.highlightedVerse
                    accessiblePrefix: "Go to verse"

                    onChosen: number => panel.jumpToVerse(number)
                }
            }

            Rectangle {
                Layout.fillHeight: true
                implicitWidth: 1
                color: LibraryStyle.divider
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                LibraryField {
                    id: search

                    leadingIcon: "search"
                    objectName: "scriptureSearch"
                    Layout.fillWidth: true
                    placeholderText: "Find a passage or search this translation…"

                    onTextChanged: panel.schedule()
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: panel.searchMode ? panel.verses.length + " search results" : (
                                                     panel.book.name || "") + " " + (
                                                     panel.chapter.chapter || 1) + " · "
                                                 + panel.verses.length + " verses"
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 13
                        color: LibraryStyle.text
                        Layout.fillWidth: true
                    }

                    LibraryButton {
                        text: "Clear search"
                        visible: panel.searchMode
                        quiet: true

                        onClicked: search.clear()
                    }

                    LibraryButton {
                        text: panel.selected.length ? "Clear selection (" + panel.selected.length
                                                      + ")" : "Select chapter"
                        quiet: true
                        enabled: panel.verses.length > 0

                        onClicked: panel.selected = panel.selected.length ? [] : panel.verses.map((
                                                                                                      verse, index)
                                                                                                  => index)
                    }
                }

                ListView {
                    id: verseView

                    objectName: "scriptureVerseList"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 7
                    model: panel.verses

                    ScrollBar.vertical: ScrollBar {
                    }
                    delegate: ItemDelegate {
                        id: verseItem

                        required property var modelData
                        required property int index
                        readonly property var secondaryMatches: panel.secondaryId
                                                                ? panel.library.passage(
                                                                      panel.secondaryId,
                                                                      modelData.book,
                                                                      modelData.chapter,
                                                                      modelData.verse,
                                                                      modelData.verse) : []
                        readonly property var secondaryVerse: secondaryMatches[0] || ({})
                        property bool chosen: panel.selected.indexOf(index) >= 0
                        property bool located: !panel.searchMode && panel.highlightedVerse
                                               === modelData.verse

                        width: ListView.view.width - 12
                        height: verseColumn.implicitHeight + 32
                        padding: 16

                        background: Rectangle {
                            color: verseItem.located ? (LibraryStyle.dark ? "#36303E" : "#EEE7F6") :
                                                       verseItem.chosen ? LibraryStyle.tone(
                                                                              "#2D2D2D") :
                                                                          verseItem.hovered
                                                                          ? LibraryStyle.tone(
                                                                                "#262626") :
                                                                            LibraryStyle.tone(
                                                                                "#222222")
                            border.color: verseItem.located ? (LibraryStyle.dark ? "#B99BCD" :
                                                                                   "#79528F") :
                                                              verseItem.chosen ? LibraryStyle.tone(
                                                                                     "#686868") :
                                                                                 LibraryStyle.tone(
                                                                                     "#303030")
                            radius: 7
                        }
                        contentItem: Column {
                            id: verseColumn

                            spacing: 8

                            Text {
                                font.family: LibraryStyle.fontFamily
                                text: (verseItem.chosen ? "Selected · " : "")
                                      + verseItem.modelData.reference
                                font.pixelSize: 11
                                color: verseItem.chosen ? LibraryStyle.tone("#E2E2E2") :
                                                          LibraryStyle.tone("#999999")
                            }

                            Text {
                                font.family: /[\u0C00-\u0C7F]/.test(text) ? "Noto Sans Telugu" :
                                                                            /[\u0900-\u097F]/.test(
                                                                                text) ? "Noto Sans Devanagari" :
                                                                                        LibraryStyle.fontFamily
                                width: parent.width
                                text: verseItem.modelData.text
                                color: LibraryStyle.tone("#E0E0E0")
                                font.pixelSize: 19
                                wrapMode: Text.Wrap
                            }

                            Text {
                                visible: !!panel.secondaryId
                                width: parent.width
                                text: (verseItem.secondaryVerse.reference || "Verse unavailable")
                                      + " · " + (verseItem.secondaryVerse.abbreviation
                                                 || "Second Bible")
                                color: LibraryStyle.muted
                                font.family: LibraryStyle.fontFamily
                                font.pixelSize: 11
                            }

                            Text {
                                visible: !!panel.secondaryId
                                width: parent.width
                                text: verseItem.secondaryVerse.text
                                      || "This reference is missing in the second Bible."
                                color: LibraryStyle.text
                                font.family: /[\u0C00-\u0C7F]/.test(text) ? "Noto Sans Telugu" :
                                                                            /[\u0900-\u097F]/.test(
                                                                                text) ? "Noto Sans Devanagari" :
                                                                                        LibraryStyle.fontFamily
                                font.pixelSize: 19
                                wrapMode: Text.Wrap
                            }
                        }

                        onClicked: panel.choose(index)
                    }

                    Text {
                        font.family: LibraryStyle.fontFamily
                        anchors.centerIn: parent
                        text: "No matching verses. Try another passage or phrase."
                        visible: panel.verses.length === 0
                        font.pixelSize: 13
                        color: LibraryStyle.tone("#888888")
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
                kind: "scripture"
                dualScripture: !!panel.secondaryId
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: LibraryStyle.tone("#333333")
        }

        RowLayout {
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    font.family: LibraryStyle.fontFamily
                    Layout.fillWidth: true
                    text: panel.version.title || ""
                    font.pixelSize: 11
                    color: LibraryStyle.tone("#BBBBBB")
                    elide: Text.ElideRight
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    Layout.fillWidth: true
                    text: (panel.version.copyright || "") + " · " + (panel.version.license || "")
                    font.pixelSize: 10
                    color: LibraryStyle.tone("#888888")
                    wrapMode: Text.Wrap
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: panel.message || panel.pairingError || panel.library.status
                    Layout.maximumWidth: 620
                    wrapMode: Text.Wrap
                    font.pixelSize: 11
                    color: LibraryStyle.tone("#AAAAAA")
                }
            }

            LibraryButton {
                text: "Source"
                quiet: true

                onClicked: Qt.openUrlExternally(panel.version.source)
            }

            Text {
                font.family: LibraryStyle.fontFamily
                text: "Lines / slide"
                color: LibraryStyle.tone("#999999")
                font.pixelSize: 11
            }

            LibrarySpinBox {
                id: lines

                from: 1
                to: 6
                value: 4
                implicitWidth: 100
            }

            ProgramAddButton {
                objectName: "bibleProgramDestination"
                visible: !!panel.programTarget
                store: panel.programTarget
                ready: panel.verses.length > 0 && !panel.pairingError

                onAddRequested: programId => panel.addToTarget(programId)
            }

            LibraryButton {
                visible: !panel.programTarget
                text: panel.selected.length ? "Add " + panel.selected.length + " verses" :
                                              "Add passage"
                iconName: "plus"
                accented: true
                enabled: panel.verses.length > 0 && !panel.pairingError

                onClicked: panel.addToTarget()
            }
        }
    }
}
