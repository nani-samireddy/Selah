pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var library
    required property var document
    property string existingId: ""

    signal saved(string templateId)

    function save(update) {
        let id = library.saveTemplate(nameField.text, kindField.model[kindField.currentIndex].kind,
                                      document.slides[document.currentSlide], update ? existingId :
                                                                                       "");
        if (id) {
            saved(id);
            close();
        }
    }

    title: "Save current slide as a template"
    modal: true
    width: 420
    anchors.centerIn: parent

    background: Rectangle {
        color: LibraryStyle.tone("#252525")
        border.color: LibraryStyle.tone("#444444")
        radius: 8
    }
    contentItem: ColumnLayout {
        spacing: 12

        LibraryField {
            id: nameField

            Layout.fillWidth: true
            placeholderText: "Template name"
        }

        LibraryCombo {
            id: kindField

            Layout.fillWidth: true
            model: [
                {
                    name: "Presentation",
                    kind: "presentation"
                },
                {
                    name: "Lyrics",
                    kind: "song"
                },
                {
                    name: "Bilingual lyrics",
                    kind: "bilingual"
                },
                {
                    name: "Scripture",
                    kind: "scripture"
                }
            ]
            textRole: "name"
        }

        Text {
            Layout.fillWidth: true
            text: kindField.model[kindField.currentIndex].kind === "presentation"
                  ? "Keep the wording, media and graphics editable for future presentations." :
                    "Bind the content boxes in the inspector. Static layers, media, colors and fonts are kept."
            color: LibraryStyle.tone("#999999")
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            text: dialog.library.status
            color: LibraryStyle.tone("#BBBBBB")
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Cancel"

                onClicked: dialog.close()
            }

            StudioButton {
                text: "Save new"

                onClicked: dialog.save(false)
            }

            StudioButton {
                text: "Update template"
                accented: true
                visible: dialog.existingId.length > 0

                onClicked: dialog.save(true)
            }
        }
    }

    onOpened: {
        let existing = library.layout(existingId);
        nameField.text = existing.name || document.slides[document.currentSlide].name;
        let slide = document.slides[document.currentSlide];
        let kind = existing.kind || (slide.elements.some(e => e.binding === "Scripture text")
                                     ? "scripture" : slide.elements.some(e => e.binding
                                                                              === "Translated lyrics")
                                       ? "bilingual" : slide.elements.some(e => e.binding
                                                                                === "Primary lyrics")
                                         ? "song" : "presentation");
        kindField.currentIndex = Math.max(0, kindField.model.findIndex(record => record.kind
                                                                                 === kind));
    }
}
