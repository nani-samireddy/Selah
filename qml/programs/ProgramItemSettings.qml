pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Service-item options stay together; all changes use the existing program commands.
ColumnLayout {
    id: settings

    required property var programView
    property alias notesText: itemNotes.text
    readonly property bool song: programView.store.selectedItem.kind === "song"
    readonly property bool hasLayout: programView.store.layoutKind.length > 0
    readonly property string templateName: programView.store.selectedItem.layoutTemplate?.name
                                           || "Automatic layout"

    Layout.fillWidth: true
    spacing: 12

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: settingsContent.implicitHeight + 24
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 8

        ColumnLayout {
            id: settingsContent

            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                LibraryCaption {
                    text: "Item settings"
                    color: LibraryStyle.text
                    Layout.fillWidth: true
                }

                LibraryButton {
                    text: "Update from editor"
                    visible: settings.programView.editingItemId.length > 0
                             && settings.programView.editingItemId
                             === settings.programView.store.selectedItem.id

                    onClicked: {
                        settings.programView.flushItem();
                        settings.programView.store.updateSelectedSlides(
                                    settings.programView.document.slides);
                    }
                }

                LibraryButton {
                    objectName: "editProgramSlidesButton"
                    text: "Edit slides"
                    iconName: "pen-tool"
                    enabled: settings.programView.store.selected >= 0

                    onClicked: {
                        settings.programView.flushItem();
                        settings.programView.editRequested(settings.programView.store.editorSlides,
                                                           settings.programView.store.selectedItem.id);
                        settings.programView.close();
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: width >= 620 ? (settings.song ? 1 : 0) + (settings.hasLayout ? 1 : 0) + 1 :
                                        width >= 400 ? 2 : 1
                columnSpacing: 12
                rowSpacing: 10

                ColumnLayout {
                    visible: settings.song
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 6

                    LibraryCaption {
                        text: "Arrangement"
                    }

                    LibraryCombo {
                        Layout.fillWidth: true
                        model: settings.programView.store.selectedItem.song?.arrangements || []
                        textRole: "name"
                        currentIndex: settings.programView.store.selectedItem.song
                                      ?.activeArrangement || 0
                        Accessible.name: "Song arrangement"

                        onActivated: index => {
                            settings.programView.flushItem();
                            settings.programView.store.setArrangement(index);
                            settings.programView.previewIndex = 0;
                        }
                    }
                }

                ColumnLayout {
                    visible: settings.hasLayout
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 6

                    LibraryCaption {
                        text: "Template"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        LibraryButton {
                            Layout.fillWidth: true
                            hint: "Choose template"
                            Accessible.name: "Template: " + settings.templateName

                            contentItem: RowLayout {
                                enabled: false
                                spacing: 6

                                StudioIcon {
                                    name: "layers"
                                    Layout.preferredWidth: 14
                                    Layout.preferredHeight: 14
                                    tone: LibraryStyle.muted
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: settings.templateName
                                    textFormat: Text.PlainText
                                    elide: Text.ElideRight
                                    font.family: LibraryStyle.fontFamily
                                    font.pixelSize: 12
                                    color: LibraryStyle.text
                                }

                                StudioIcon {
                                    name: "chevron-down"
                                    Layout.preferredWidth: 12
                                    Layout.preferredHeight: 12
                                    tone: LibraryStyle.muted
                                }
                            }

                            onClicked: {
                                settings.programView.flushItem();
                                settings.programView.templatesRequested();
                            }
                        }

                        LibraryButton {
                            visible: !!settings.programView.store.selectedItem.layoutTemplate?.id
                            iconName: "rotate-ccw"
                            implicitWidth: 28
                            hint: "Reset to automatic layout"

                            onClicked: {
                                settings.programView.flushItem();
                                settings.programView.store.clearLayout();
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.alignment: Qt.AlignTop
                    spacing: 6

                    LibraryCaption {
                        text: "Transition"
                    }

                    TransitionField {
                        Layout.fillWidth: true
                        controlName: "programItemTransition"
                        enabled: settings.programView.store.selected >= 0
                        allowInherit: true
                        inheritLabel: "As authored"
                        transition: settings.programView.store.selectedItem.transition || ({})

                        onChosen: (type, durationMs) => {
                            settings.programView.flushItem();
                            settings.programView.store.setTransition(type, durationMs);
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true

        LibraryCaption {
            text: "Speaker notes"
            Layout.fillWidth: true
        }

        Text {
            text: "Not shown to the audience"
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 10
            color: LibraryStyle.muted
        }
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.preferredHeight: 64
        Layout.minimumHeight: 45
        clip: true

        TextArea {
            id: itemNotes

            objectName: "programItemNotes"
            enabled: settings.programView.store.selected >= 0
            placeholderText: "Add notes for the speaker…"
            placeholderTextColor: LibraryStyle.muted
            color: LibraryStyle.text
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
            padding: 10
            wrapMode: TextEdit.Wrap
            selectByMouse: true

            background: Rectangle {
                color: LibraryStyle.field
                border.color: itemNotes.activeFocus ? LibraryStyle.focus : LibraryStyle.border
                radius: 8
            }

            onTextChanged: if (!settings.programView.loading && activeFocus)
                               settings.programView.markItemDirty()
        }
    }
}
