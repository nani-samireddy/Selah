pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: editor

    required property var sections
    required property var order

    signal edited(var keys)
    signal editSection(int index)

    function move(from, delta) {
        let to = from + delta;
        if (to < 0 || to >= order.length)
            return;
        let keys = order.slice();
        keys.splice(to, 0, keys.splice(from, 1)[0]);
        edited(keys);
    }

    function remove(index) {
        let keys = order.slice();
        keys.splice(index, 1);
        edited(keys);
    }

    spacing: 8

    RowLayout {
        Layout.fillWidth: true

        LibraryCaption {
            text: "Song order"
            Layout.fillWidth: true
        }

        LibraryCombo {
            id: addSection

            Layout.preferredWidth: 170
            model: editor.sections
            textRole: "label"
            displayText: "Add section…"
            currentIndex: -1
            Accessible.name: "Add a section to the song order"

            onActivated: index => {
                editor.edited(editor.order.concat([editor.sections[index].key]));
                currentIndex = -1;
            }
        }
    }

    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        spacing: 6

        Repeater {
            model: editor.order

            LibraryButton {
                id: chip

                required property int index
                required property string modelData
                readonly property int sectionIndex: editor.sections.findIndex(section
                                                                              => section.key
                                                                                 === modelData)

                text: (index + 1) + " · " + (editor.sections[sectionIndex]?.label || modelData)
                hint: text + " — edit, move, or remove"

                onClicked: actions.open()

                Popup {
                    id: actions

                    y: chip.height + 4
                    padding: 4

                    background: Rectangle {
                        color: LibraryStyle.tone("#202020")
                        border.color: LibraryStyle.border
                        radius: 6
                    }
                    contentItem: Column {
                        spacing: 2

                        LibraryButton {
                            text: "Edit lyrics"
                            quiet: true
                            width: 160

                            onClicked: {
                                actions.close();
                                editor.editSection(chip.sectionIndex);
                            }
                        }

                        LibraryButton {
                            text: "Move earlier"
                            quiet: true
                            width: 160
                            enabled: chip.index > 0

                            onClicked: {
                                actions.close();
                                editor.move(chip.index, -1);
                            }
                        }

                        LibraryButton {
                            text: "Move later"
                            quiet: true
                            width: 160
                            enabled: chip.index < editor.order.length - 1

                            onClicked: {
                                actions.close();
                                editor.move(chip.index, 1);
                            }
                        }

                        LibraryButton {
                            text: "Remove from order"
                            quiet: true
                            width: 160

                            onClicked: {
                                actions.close();
                                editor.remove(chip.index);
                            }
                        }
                    }
                }
            }
        }
    }

    Text {
        visible: editor.order.length === 0
        text: "Add sections in the order you want to sing them."
        color: LibraryStyle.muted
        font.family: LibraryStyle.fontFamily
        font.pixelSize: 12
    }
}
