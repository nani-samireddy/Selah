pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var library
    property string primaryLanguage: "English"
    property string errorMessage: ""

    signal songCreated(string songId)

    function saveSong() {
        tagsEditor.commit();
        const id = library.createQuickSong(nameField.text, lyrics.text, tagsEditor.tags,
                                           primaryLanguage);
        if (!id) {
            errorMessage = library.status;
            return;
        }
        close();
        songCreated(id);
    }

    objectName: "quickSongDialog"
    title: "Quick Song"
    modal: true
    focus: true
    z: 10
    anchors.centerIn: parent
    width: Math.min(660, parent.width - 48)
    height: Math.min(640, parent.height - 48)
    padding: 20

    background: Rectangle {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 10
    }
    contentItem: ColumnLayout {
        spacing: 10

        LibraryCaption {
            text: "Name"
        }

        LibraryField {
            id: nameField

            objectName: "quickSongName"
            Layout.fillWidth: true
            placeholderText: "Song name"
            Accessible.name: "Quick Song name"
        }

        LibraryCaption {
            text: "Lyrics"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            LibraryTextArea {
                id: lyrics

                objectName: "quickSongLyrics"
                placeholderText: "Paste or type the song lyrics…"
                Accessible.name: "Quick Song lyrics"
            }
        }

        LibraryCaption {
            text: "Leave a blank line to start a new section."
        }

        SongTagEditor {
            id: tagsEditor

            objectPrefix: "quickSong"
            library: dialog.library
            Layout.fillWidth: true
        }

        Text {
            Layout.fillWidth: true
            visible: dialog.errorMessage.length > 0
            text: dialog.errorMessage
            color: LibraryStyle.text
            wrapMode: Text.Wrap
            font.pixelSize: 12
        }

        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Cancel"

                onClicked: dialog.close()
            }

            LibraryButton {
                objectName: "quickSongSave"
                text: "Save song"
                accented: true
                enabled: nameField.text.trim().length > 0 && lyrics.text.trim().length > 0

                onClicked: dialog.saveSong()
            }
        }
    }

    onOpened: {
        nameField.clear();
        lyrics.clear();
        tagsEditor.reset([]);
        errorMessage = "";
        nameField.forceActiveFocus();
    }
}
