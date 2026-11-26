pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: browser

    required property var host
    required property string section
    property var rows: []
    // Dialogs are declared in the parent so opening them does not clear the preview.
    property var mediaFile
    property var presentationFile

    signal chosen(var record)

    function refresh() {
        if (section === "songs") {
            rows = host.quickAdd.searchSongs(search.text);
        } else {
            const source = section === "media" ? host.library.assets :
                                                 host.presentations.presentations;
            const query = search.text.trim().toLocaleLowerCase();
            rows = source.filter(row => (row.title || row.name || "").toLocaleLowerCase().includes(
                        query));
        }
    }

    onSectionChanged: {
        search.clear();
        refresh();
    }
    Component.onCompleted: refresh()

    RowLayout {
        Layout.fillWidth: true

        LibraryField {
            id: search

            objectName: "quickAddSearch"
            Layout.fillWidth: true
            placeholderText: browser.section === "songs" ? "Search song names or lyrics…" :
                                                           "Search library…"

            onTextChanged: debounce.restart()
        }

        LibraryButton {
            visible: browser.section !== "songs"
            text: "Import…"
            iconName: "upload"

            onClicked: browser.section === "media" ? browser.mediaFile.open() :
                                                     browser.presentationFile.open()
        }
    }

    Timer {
        id: debounce

        interval: 150

        onTriggered: browser.refresh()
    }

    ListView {
        id: results

        objectName: "quickAddResults"
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 4
        model: browser.rows

        ScrollBar.vertical: ScrollBar {
        }
        delegate: ItemDelegate {
            id: row

            required property var modelData

            width: results.width
            height: 52
            padding: 10

            background: SquircleSurface {
                color: row.hovered ? LibraryStyle.hover : LibraryStyle.field
                radius: 6
            }
            contentItem: Column {
                spacing: 4

                Text {
                    text: row.modelData.title || row.modelData.name || "Untitled"
                    color: LibraryStyle.text
                    font.pixelSize: 13
                    width: parent.width
                    elide: Text.ElideRight
                }

                Text {
                    text: browser.section === "songs" ? row.modelData.primaryLanguage || "Song" :
                                                        browser.section === "media"
                                                        ? row.modelData.kind : "Presentation"
                    color: LibraryStyle.muted
                    font.pixelSize: 11
                }
            }

            onClicked: browser.chosen(modelData)
        }

        Text {
            anchors.centerIn: parent
            visible: browser.rows.length === 0
            text: "No matches. Try another search or add new content."
            color: LibraryStyle.muted
            width: parent.width - 20
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
        }
    }
}
