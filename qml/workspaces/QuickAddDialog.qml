pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog

    required property var host
    readonly property var controller: host.quickAdd
    property string section: "songs"
    property int slideIndex: 0
    readonly property var slide: controller.slides[slideIndex] || ({})
    readonly property bool draftMode: section === "quickSong" || section === "text"

    function remember(key, value) {
        const next = Object.assign({}, controller.draft);
        next[key] = value;
        controller.draft = next;
    }

    function finish(now) {
        if (controller.add(now)) {
            host.notify(controller.status);
            close();
        }
    }

    objectName: "quickAddDialog"
    title: "Quick Add"
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(1040, parent.width - 40)
    height: Math.min(740, parent.height - 40)
    padding: 20
    standardButtons: Dialog.Close

    background: SquircleSurface {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 12
    }
    contentItem: ColumnLayout {
        spacing: 14

        LibraryCaption {
            text: "Find or create content without leaving the live presentation."
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        LibraryTabs {
            currentValue: dialog.section
            options: [
                {
                    text: "Songs",
                    value: "songs"
                },
                {
                    text: "Bible",
                    value: "bible"
                },
                {
                    text: "Quick Song",
                    value: "quickSong"
                },
                {
                    text: "Text",
                    value: "text"
                },
                {
                    text: "Media",
                    value: "media"
                },
                {
                    text: "Presentations",
                    value: "presentations"
                }
            ]

            onChosen: value => dialog.section = value
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true

                QuickAddBrowser {
                    id: browser

                    host: dialog.host
                    section: dialog.section
                    mediaFile: mediaPicker
                    presentationFile: presentationPicker
                    visible: !dialog.draftMode && dialog.section !== "bible"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    onChosen: record => {
                        if (dialog.section === "songs")
                            dialog.controller.previewSong(record.id);
                        else if (dialog.section === "media")
                            dialog.controller.previewMedia(record);
                        else
                            dialog.controller.previewPresentation(record.id);
                    }
                }

                ColumnLayout {
                    visible: dialog.section === "bible"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    LibraryCaption {
                        text: "Bible"
                    }

                    LibraryCombo {
                        id: primary

                        objectName: "quickAddBible"
                        Layout.fillWidth: true
                        model: dialog.host.content.translations
                        textRole: "title"
                        currentIndex: {
                            const index = model.findIndex(row => row.id
                                                                 === dialog.controller.draft.translation);


                            return index >= 0 ? index : (model.length ? 0 : -1);
                        }

                        onActivated: index => dialog.remember("translation", model[index].id)
                    }

                    LibraryCaption {
                        text: "Second Bible (optional)"
                    }

                    LibraryCombo {
                        id: secondary

                        Layout.fillWidth: true
                        model: [
                            {
                                id: "",
                                title: "None (single Bible)"
                            }
                        ].concat(dialog.host.content.translations)
                        textRole: "title"
                        currentIndex: Math.max(0, model.findIndex(row => row.id
                                                                         === dialog.controller.draft.secondary))

                        onActivated: index => dialog.remember("secondary", model[index].id)
                    }

                    LibraryCaption {
                        text: "Reference or words"
                    }

                    LibraryField {
                        id: reference

                        objectName: "quickAddReference"
                        Layout.fillWidth: true
                        placeholderText: "John 3:16–18"

                        onTextEdited: dialog.controller.clearPreview()
                        onAccepted: passagePreview.clicked()
                    }

                    LibraryButton {
                        id: passagePreview

                        objectName: "quickAddPassagePreview"
                        text: "Preview passage"
                        iconName: "book-open"
                        enabled: primary.currentIndex >= 0 && reference.text.trim().length > 0

                        onClicked: dialog.controller.previewBible(
                                       primary.model[primary.currentIndex].id, reference.text,
                                       secondary.model[secondary.currentIndex].id)
                    }

                    LibraryCaption {
                        text: "Use a verse range to follow a longer reading. Spelling suggestions use installed book names."
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }

                ColumnLayout {
                    visible: dialog.draftMode
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    LibraryField {
                        objectName: "quickAddDraftTitle"
                        Layout.fillWidth: true
                        placeholderText: dialog.section === "quickSong" ? "Song name" : "Title"
                        text: dialog.controller.draft.title || ""

                        onTextEdited: dialog.remember("title", text)
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: availableWidth
                        clip: true

                        LibraryTextArea {
                            objectName: "quickAddDraftText"
                            text: dialog.controller.draft.text || ""
                            placeholderText: dialog.section === "quickSong" ? "Paste lyrics…" :
                                                                              "Enter the message…"

                            onTextChanged: if (activeFocus)
                                               dialog.remember("text", text)
                        }
                    }

                    LibraryField {
                        visible: dialog.section === "quickSong"
                        Layout.fillWidth: true
                        placeholderText: "Tags (optional)"
                        text: dialog.controller.draft.tags || ""

                        onTextEdited: dialog.remember("tags", text)
                    }

                    SongLanguageCombo {
                        visible: dialog.section === "quickSong"
                        Layout.fillWidth: true
                        value: dialog.controller.draft.language || "English"

                        onChosen: dialog.remember("language", value)
                    }

                    LibraryCaption {
                        text: "Blank lines start new sections. Draft saved on this computer."
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    LibraryButton {
                        objectName: "quickAddDraftPreview"
                        text: "Preview"

                        onClicked: dialog.controller.previewDraft(dialog.section === "quickSong")
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1.4
                Layout.fillHeight: true
                spacing: 10

                Text {
                    text: dialog.controller.title || "Preview"
                    color: LibraryStyle.text
                    font.pixelSize: 15
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Rectangle {
                    id: preview

                    Layout.fillWidth: true
                    Layout.preferredHeight: width * 9 / 16
                    color: "#090909"
                    radius: 7
                    border.color: LibraryStyle.border
                    clip: true

                    SlideRenderer {
                        visible: dialog.controller.slides.length > 0
                        scale: preview.width / 1920
                        transformOrigin: Item.TopLeft
                        elements: dialog.slide.elements || []
                        backgroundColor: dialog.slide.background || "#000000"
                        backgroundFill: dialog.slide.backgroundFill || ({})
                        ambientOpacity: dialog.slide.ambientOpacity === undefined ? 0 :
                                                                                    dialog.slide.ambientOpacity
                        videoEnabled: dialog.opened
                        playing: false
                        audible: false
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: dialog.controller.slides.length === 0
                        text: "Select content to preview"
                        color: "#AAAAAA"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    LibraryButton {
                        iconName: "chevron-left"
                        hint: "Previous preview slide"
                        enabled: dialog.slideIndex > 0

                        onClicked: --dialog.slideIndex
                    }

                    LibraryCaption {
                        text: dialog.controller.slides.length ? (dialog.slideIndex + 1) + " / "
                                                                + dialog.controller.slides.length :
                                                                "No slides"
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    LibraryButton {
                        iconName: "chevron-right"
                        hint: "Next preview slide"
                        enabled: dialog.slideIndex + 1 < dialog.controller.slides.length

                        onClicked: ++dialog.slideIndex
                    }
                }

                LibraryCaption {
                    text: dialog.controller.status
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }

                LibraryButton {
                    text: "Save to library"
                    iconName: "plus"
                    visible: dialog.draftMode
                    enabled: dialog.controller.canSaveDraft

                    onClicked: dialog.controller.saveDraft()
                }

                Item {
                    Layout.fillHeight: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    LibraryButton {
                        objectName: "quickAddInsertNext"
                        text: "Insert next"
                        iconName: "list-ordered"
                        enabled: dialog.controller.slides.length > 0
                                 && dialog.host.outputs.liveCount > 0
                        Layout.fillWidth: true

                        onClicked: dialog.finish(false)
                    }

                    LibraryButton {
                        objectName: "quickAddPresentNow"
                        text: "Present now"
                        iconName: "play"
                        accented: true
                        enabled: dialog.controller.slides.length > 0
                        Layout.fillWidth: true

                        onClicked: dialog.finish(true)
                    }
                }
            }
        }
    }

    onSectionChanged: controller.clearPreview()
    onOpened: {
        controller.clearPreview();
        browser.refresh();
    }

    Connections {
        function onChanged() {
            dialog.slideIndex = 0;
        }

        target: dialog.controller
    }

    FileDialog {
        id: mediaPicker

        title: "Choose an image, video or audio"
        nameFilters:
            ["Media (*.png *.jpg *.jpeg *.webp *.gif *.bmp *.svg *.mp4 *.mov *.webm *.mkv *.m4v *.mp3 *.wav *.m4a *.aac *.flac *.ogg *.aiff *.aif *.opus)"]

        onAccepted: dialog.controller.importFile(selectedFile, false)
    }

    FileDialog {
        id: presentationPicker

        title: "Choose a Selah presentation"
        nameFilters: ["Selah presentation (*.json *.selah *.churchdeck)", "All files (*)"]

        onAccepted: dialog.controller.importFile(selectedFile, true)
    }
}
