pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

// The draft owns its tags. Suggestions come only from saved library metadata.
ColumnLayout {
    id: editor

    required property var library
    property var tags: []
    property string objectPrefix: "song"
    readonly property var suggestions: library.songTags.filter(tag => entry.text.trim() && tag.toLocaleLowerCase(
                                                                          ).includes(entry.text.trim(
                                                                                         ).toLocaleLowerCase(
                                                                                         )) && !tags.some(
                                                                          existing
                                                                          => existing.toLocaleLowerCase(
                                                                                 ) === tag.toLocaleLowerCase(
                                                                                 ))).slice(0, 6)

    signal edited

    function commit() {
        const incoming = library.parseSongTags(entry.text);
        if (!incoming.length)
            return;
        tags = library.parseSongTags(tags.concat(incoming));
        entry.clear();
        edited();
    }

    function reset(value) {
        tags = library.parseSongTags(value);
        entry.clear();
    }

    spacing: 5

    Flow {
        Layout.fillWidth: true
        spacing: 6

        LibraryCaption {
            text: "Tags"
            height: LibraryStyle.controlHeight
            verticalAlignment: Text.AlignVCenter
        }

        Repeater {
            model: editor.tags

            LibraryButton {
                id: chip

                required property string modelData
                required property int index

                objectName: editor.objectPrefix + "TagChip_" + index
                text: modelData + "  ×"
                hint: "Remove tag " + modelData
                selected: true
                implicitHeight: LibraryStyle.controlHeight
                implicitWidth: Math.min(240, tagLabel.implicitWidth + 22)

                contentItem: Text {
                    id: tagLabel

                    text: chip.text
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 12
                    color: LibraryStyle.text
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                onClicked: {
                    editor.tags = editor.tags.filter((tag, i) => i !== index);
                    editor.edited();
                }
            }
        }

        LibraryField {
            id: entry

            objectName: editor.objectPrefix + "TagInput"
            width: 180
            placeholderText: "Add a tag…"
            Accessible.name: "Song tags, separated by commas"

            onTextEdited: editor.edited()
            onAccepted: editor.commit()
        }

        LibraryButton {
            objectName: editor.objectPrefix + "TagAdd"
            implicitWidth: 32
            iconName: "plus"
            hint: "Add tag"
            enabled: entry.text.trim().length > 0

            onClicked: editor.commit()
        }
    }

    Flow {
        Layout.fillWidth: true
        visible: editor.suggestions.length > 0
        spacing: 5

        Repeater {
            model: editor.suggestions

            LibraryButton {
                required property string modelData

                text: modelData
                implicitHeight: 26
                quiet: true
                hint: "Use tag " + modelData

                onClicked: {
                    editor.tags = editor.library.parseSongTags(editor.tags.concat([modelData]));
                    entry.clear();
                    editor.edited();
                }
            }
        }
    }
}
