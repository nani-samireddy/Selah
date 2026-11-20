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
    property var versions: library.translations
    property var version: versions[versionPicker.currentIndex] || ({})
    property var bookList: library.books(version.id || "")
    property var book: bookList[bookPicker.currentIndex] || ({})
    property var chapterList: library.chapters(version.id || "", book.code || "")
    property var chapter: chapterList[chapterPicker.currentIndex] || ({})
    property var verses: []
    property var selected: []
    property string message: ""
    property bool searchMode: search.text.trim().length > 0

    function refresh() {
        selected = [];
        message = "";
        verses = searchMode ? library.searchVerses(version.id || "", search.text) : library.passage(
                                  version.id || "", book.code || "", chapter.chapter || 1,
                                  fromVerse.value, toVerse.value);
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
                chapterPicker.currentIndex = i;
        Qt.callLater(function () {
            fromVerse.value = first;
            toVerse.value = last;
            panel.refresh();
        });
    }

    width: Math.min(1000, parent.width - 40)
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

    onOpened: refresh()
    onVersionChanged: schedule()
    onBookChanged: schedule()
    onChapterChanged: Qt.callLater(function () {
        fromVerse.value = panel.chapter.first || 1;
        toVerse.value = panel.chapter.last || 1;
        panel.schedule();
    })

    Timer {
        id: debounce

        interval: 230

        onTriggered: panel.refresh()
    }

    FileDialog {
        id: importDialog

        title: "Import Bible translation"
        nameFilters: ["Church Deck Bible packages (*.json)"]

        onAccepted: panel.library.importBible(selectedFile)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        RowLayout {
            Column {
                spacing: 5

                Text {
                    text: "Bible"
                    color: "#EEEEEE"
                    font.pixelSize: 21
                    font.weight: Font.Medium
                }

                Text {
                    text: "Find a passage. Bring the Word to the screen."
                    color: "#929292"
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StudioButton {
                text: panel.library.busy ? "Importing…" : "Import translation"
                enabled: !panel.library.busy

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
            spacing: 12

            LibraryCombo {
                id: versionPicker

                Layout.preferredWidth: 270
                model: panel.versions
                textRole: "abbreviation"

                onActivated: {
                    bookPicker.currentIndex = 0;
                    chapterPicker.currentIndex = 0;
                    panel.refresh();
                }
            }

            LibraryField {
                id: search

                objectName: "scriptureSearch"
                Layout.fillWidth: true
                placeholderText: "John 3:16–18, or words in this translation…"

                onTextChanged: panel.schedule()
            }

            StudioButton {
                text: "Clear"
                visible: panel.searchMode
                quiet: true

                onClicked: search.clear()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            enabled: !panel.searchMode

            LibraryCombo {
                id: bookPicker

                Layout.fillWidth: true
                model: panel.bookList
                textRole: "name"

                onActivated: {
                    chapterPicker.currentIndex = 0;
                    panel.refresh();
                }
            }

            Text {
                text: "Chapter"
                color: "#999999"
                font.pixelSize: 12
            }

            LibraryCombo {
                id: chapterPicker

                Layout.preferredWidth: 80
                model: panel.chapterList
                textRole: "chapter"

                onActivated: panel.refresh()
            }

            Text {
                text: "Verses"
                color: "#999999"
                font.pixelSize: 12
            }

            SpinBox {
                id: fromVerse

                from: panel.chapter.first || 1
                to: panel.chapter.last || 1
                value: from
                implicitWidth: 100

                onValueModified: {
                    if (toVerse.value < value)
                        toVerse.value = value;
                    panel.refresh();
                }
            }

            Text {
                text: "–"
                color: "#999999"
            }

            SpinBox {
                id: toVerse

                from: fromVerse.value
                to: panel.chapter.last || 1
                value: to
                implicitWidth: 100

                onValueModified: panel.refresh()
            }
        }

        RowLayout {
            Text {
                text: panel.searchMode ? panel.verses.length + " results · First 100 matches" : (
                                             panel.book.name || "") + " " + (panel.chapter.chapter
                                                                             || "") + " · "
                                         + panel.verses.length + " verses"
                color: "#999999"
                font.pixelSize: 11
                Layout.fillWidth: true
            }

            StudioButton {
                text: panel.selected.length ? "Clear selection" : "Select all"
                quiet: true
                implicitHeight: 28
                enabled: panel.verses.length > 0

                onClicked: {
                    panel.selected = panel.selected.length ? [] : panel.verses.map((v, i) => i);
                }
            }
        }

        ListView {
            id: verseView

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
                property bool chosen: panel.selected.indexOf(index) >= 0

                width: ListView.view.width - 12
                height: verseColumn.implicitHeight + 26

                background: Rectangle {
                    color: verseItem.chosen ? "#30343E" : verseItem.hovered ? "#262626" : "#222222"
                    border.color: verseItem.chosen ? "#777D8A" : "#303030"
                    radius: 7
                }
                contentItem: Column {
                    id: verseColumn

                    spacing: 8

                    Text {
                        text: (verseItem.chosen ? "Selected · " : "")
                              + verseItem.modelData.reference
                        font.pixelSize: 11
                        color: verseItem.chosen ? "#CFD5E0" : "#999999"
                    }

                    Text {
                        width: parent.width
                        text: verseItem.modelData.text
                        color: "#E0E0E0"
                        font.pixelSize: 19
                        font.family: /[\u0C00-\u0C7F]/.test(text) ? "Noto Sans Telugu" :
                                                                    /[\u0900-\u097F]/.test(text)
                                                                    ? "Noto Sans Devanagari" :
                                                                      "Arial"
                        wrapMode: Text.Wrap
                    }
                }

                onClicked: panel.choose(index)
            }

            Text {
                anchors.centerIn: parent
                text: "No matching verses. Try another passage or phrase."
                visible: panel.verses.length === 0
                font.pixelSize: 13
                color: "#888888"
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
                kind: "scripture"
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#333333"
        }

        RowLayout {
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: panel.version.title || ""
                    font.pixelSize: 11
                    color: "#BBBBBB"
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: (panel.version.copyright || "") + " · " + (panel.version.license || "")
                    font.pixelSize: 10
                    color: "#888888"
                    wrapMode: Text.Wrap
                }

                Text {
                    text: panel.message || panel.library.status
                    font.pixelSize: 11
                    color: "#AAAAAA"
                }
            }

            StudioButton {
                text: "Source"
                quiet: true

                onClicked: Qt.openUrlExternally(panel.version.source)
            }

            Text {
                text: "Lines / slide"
                color: "#999999"
                font.pixelSize: 11
            }

            SpinBox {
                id: lines

                from: 1
                to: 6
                value: 4
                implicitWidth: 100
            }

            StudioButton {
                text: panel.programTarget ? "Add to program" : panel.selected.length ? "Add "
                                                                                       + panel.selected.length
                                                                                       + " verses" :
                                                                                       "Add passage"
                iconName: "plus"
                accented: true
                enabled: panel.verses.length > 0

                onClicked: {
                    let rows = panel.selected.length ? panel.selected.map(i => panel.verses[i]) :
                                                       panel.verses;
                    let target = panel.programTarget || panel.document;
                    if (target.addScriptureSlides ? target.addScriptureSlides(rows, lines.value,
                                                                              templateChoice.definition) :
                                                    target.addScripture(rows, lines.value,
                                                                        templateChoice.definition))
                        panel.close();
                    else
                        panel.message = target.status;
                }
            }
        }
    }
}
