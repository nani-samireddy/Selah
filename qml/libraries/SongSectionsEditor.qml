pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Stable keyed delegates let every section remain visible without interrupting typing.
GridLayout {
    id: editor

    required property var sections
    required property string primaryLanguage
    required property string secondaryLanguage
    required property var fontForText
    required property var library
    property bool chordsMode: false
    property bool showDetails: false
    property bool syncing: false
    property bool ready: false

    signal edited(var sections)
    signal focused(int index)

    function sectionItem(index) {
        return sectionRepeater.itemAt(index);
    }

    function sync() {
        if (!ready || syncing)
            return;
        syncing = true;
        for (let i = 0; i < sections.length; i++) {
            if (i >= cards.count)
                cards.append({
                                 payload: sections[i]
                             });
            else if (JSON.stringify(cards.get(i).payload) !== JSON.stringify(sections[i]))
                cards.setProperty(i, "payload", sections[i]);
        }
        if (cards.count > sections.length)
            cards.remove(sections.length, cards.count - sections.length);
        syncing = false;
    }

    function updateSection(index, key, value) {
        if (syncing || index < 0 || index >= cards.count)
            return;
        let row = Object.assign({}, cards.get(index).payload);
        if (row[key] === value)
            return;
        row[key] = value;
        syncing = true;
        cards.setProperty(index, "payload", row);
        let result = [];
        for (let i = 0; i < cards.count; i++)
            result.push(Object.assign({}, cards.get(i).payload));
        edited(result);
        syncing = false;
    }

    columns: secondaryLanguage || width < 760 ? 1 : 2
    columnSpacing: 12
    rowSpacing: 12

    onSectionsChanged: sync()
    Component.onCompleted: {
        ready = true;
        sync();
    }

    // Inline chord tokens remain editable and travel with each section.
    ListModel {
        id: cards

        dynamicRoles: true
    }

    Repeater {
        id: sectionRepeater

        model: cards

        SquircleSurface {
            id: card

            required property int index
            required property var payload

            function focusLyrics() {
                primary.forceActiveFocus();
            }

            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            implicitHeight: contents.implicitHeight + 24
            color: LibraryStyle.field
            border.color: primary.activeFocus || secondary.activeFocus ? LibraryStyle.focus :
                                                                         LibraryStyle.border
            radius: 8

            ColumnLayout {
                id: contents

                spacing: 10

                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: 12
                }

                Text {
                    visible: !editor.showDetails
                    text: card.payload.label
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: LibraryStyle.text
                }

                LibraryField {
                    visible: editor.showDetails
                    Layout.fillWidth: true
                    text: card.payload.label
                    placeholderText: "Section name"

                    onTextEdited: editor.updateSection(card.index, "label", text)
                }

                LibraryField {
                    visible: editor.chordsMode
                    Layout.fillWidth: true
                    placeholderText: "Stage cue, e.g. Repeat chorus / Instrumental"
                    text: card.payload.stageCue || ""

                    onTextEdited: editor.updateSection(card.index, "stageCue", text)
                }

                Text {
                    visible: editor.chordsMode
                    Layout.fillWidth: true
                    text: "Add chords before words: [C]Amazing [G]grace"
                    color: LibraryStyle.muted
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                }

                ChordLyricsItem {
                    visible: editor.chordsMode
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(110, Math.min(300, (card.payload.primary || "").split(
                                                                       "\n").length * 52))
                    content: editor.library.chordPreview(card.payload.primary || "")
                    pixelSize: 24
                    color: LibraryStyle.text
                    chordColor: LibraryStyle.dark ? "#E8C987" : "#79520E"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1

                        Text {
                            visible: !!editor.secondaryLanguage
                            text: editor.primaryLanguage
                            color: LibraryStyle.muted
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 11
                        }

                        LibraryTextArea {
                            id: primary

                            objectName: card.index === 0 ? "primaryLyrics" : "primaryLyrics_"
                                                           + card.index
                            Layout.fillWidth: true
                            implicitHeight: Math.max(96, contentHeight + 12)
                            text: card.payload.primary || ""
                            placeholderText: "Write the lyrics for "
                                             + card.payload.label.toLowerCase() + "…"
                            font.family: editor.fontForText(text)
                            font.pixelSize: 15
                            padding: 0

                            background: Item {
                            }

                            onTextChanged: editor.updateSection(card.index, "primary", text)
                            onActiveFocusChanged: if (activeFocus)
                                                      editor.focused(card.index)
                        }
                    }

                    ColumnLayout {
                        visible: !!editor.secondaryLanguage || !!card.payload.secondary
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1

                        Text {
                            text: editor.secondaryLanguage || "Translation"
                            color: LibraryStyle.muted
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 11
                        }

                        LibraryTextArea {
                            id: secondary

                            objectName: card.index === 0 ? "secondaryLyrics" : "secondaryLyrics_"
                                                           + card.index
                            Layout.fillWidth: true
                            implicitHeight: Math.max(96, contentHeight + 12)
                            text: card.payload.secondary || ""
                            placeholderText: "Matching translation…"
                            font.family: editor.fontForText(text)
                            font.pixelSize: 15
                            padding: 0

                            background: Item {
                            }

                            onTextChanged: editor.updateSection(card.index, "secondary", text)
                            onActiveFocusChanged: if (activeFocus)
                                                      editor.focused(card.index)
                        }
                    }
                }
            }
        }
    }
}
