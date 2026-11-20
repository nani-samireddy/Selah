pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: itemEditor

    required property var programView
    property alias titleText: itemTitle.text
    property alias notesText: itemNotes.text

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 8

    RowLayout {
        Text {
            text: "PREVIEW"
            font.pixelSize: 10
            font.letterSpacing: 1
            color: "#888888"
            Layout.fillWidth: true
        }

        Text {
            text: itemEditor.programView.store.selectedSlides.length ? (
                                                                           itemEditor.programView.previewIndex
                                                                           + 1) + " / "
                                                                       + itemEditor.programView.store.selectedSlides.length :
                                                                       ""
            font.pixelSize: 11
            color: "#999999"
        }

        StudioButton {
            text: "Take slide live"
            accented: true
            enabled: itemEditor.programView.store.selectedSlides.length > 0

            onClicked: itemEditor.programView.takeLive()
        }
    }

    Rectangle {
        id: draftFrame

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: width * 9 / 16
        Layout.maximumHeight: 285
        Layout.minimumHeight: 100
        color: "#111111"
        border.color: "#363636"
        radius: 5
        clip: true

        SlideRenderer {
            width: 1920
            height: 1080
            scale: Math.min(draftFrame.width / 1920, draftFrame.height / 1080)
            transformOrigin: Item.TopLeft
            x: (draftFrame.width - width * scale) / 2
            y: (draftFrame.height - height * scale) / 2
            elements: itemEditor.programView.previewSlide.elements || []
            backgroundColor: itemEditor.programView.previewSlide.background || "#111111"
            ambientOpacity: itemEditor.programView.previewSlide.ambientOpacity === undefined ? 0.5 :
                                                                                               itemEditor.programView.previewSlide.ambientOpacity
            videoEnabled: false
            playing: false
        }

        Text {
            anchors.centerIn: parent
            visible: !itemEditor.programView.store.selectedSlides.length
            text: "Select a service item"
            color: "#777777"
            font.pixelSize: 13
        }
    }

    ListView {
        id: slideList

        Layout.fillWidth: true
        Layout.preferredHeight: 72
        orientation: ListView.Horizontal
        spacing: 8
        clip: true
        model: itemEditor.programView.store.selectedSlides

        ScrollBar.horizontal: ScrollBar {
        }
        delegate: Rectangle {
            id: slideThumb

            required property var modelData
            required property int index

            width: 112
            height: 66
            color: "#111111"
            radius: 5
            border.width: 2
            border.color: index === itemEditor.programView.previewIndex ? "#AAAAAA" : "#383838"
            clip: true

            SlideRenderer {
                x: 3
                y: 3
                width: 1920
                height: 1080
                scale: 106 / 1920
                transformOrigin: Item.TopLeft
                elements: slideThumb.modelData.elements || []
                backgroundColor: slideThumb.modelData.background
                ambientOpacity: slideThumb.modelData.ambientOpacity === undefined ? 0.5 :
                                                                                    slideThumb.modelData.ambientOpacity
                videoEnabled: false
            }

            MouseArea {
                anchors.fill: parent

                onClicked: itemEditor.programView.previewIndex = slideThumb.index
                onDoubleClicked: {
                    itemEditor.programView.previewIndex = slideThumb.index;
                    itemEditor.programView.takeLive();
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true

        LibraryField {
            id: itemTitle

            objectName: "programItemTitle"
            Layout.fillWidth: true
            enabled: itemEditor.programView.store.selected >= 0
            placeholderText: "Item title"

            onTextEdited: {
                itemEditor.programView.markItemDirty();
            }
        }

        StudioButton {
            text: "Edit slides"
            enabled: itemEditor.programView.store.selected >= 0

            onClicked: {
                itemEditor.programView.flushItem();
                itemEditor.programView.editRequested(itemEditor.programView.store.editorSlides,
                                                     itemEditor.programView.store.selectedItem.id);
                itemEditor.programView.close();
            }
        }

        StudioButton {
            text: "Update from editor"
            visible: itemEditor.programView.editingItemId.length > 0
                     && itemEditor.programView.editingItemId
                     === itemEditor.programView.store.selectedItem.id

            onClicked: {
                itemEditor.programView.flushItem();
                itemEditor.programView.store.updateSelectedSlides(
                            itemEditor.programView.document.slides);
            }
        }
    }

    RowLayout {
        visible: itemEditor.programView.store.selectedItem.kind === "song"
                 || itemEditor.programView.store.selectedItem.kind === "scripture"

        Text {
            text: "Style"
            color: "#999999"
            font.pixelSize: 11
        }

        LibraryCombo {
            Layout.preferredWidth: 155
            model: [
                {
                    label: "Original",
                    id: "original"
                },
                {
                    label: "Midnight",
                    id: "midnight"
                },
                {
                    label: "Warm paper",
                    id: "paper"
                }
            ]
            textRole: "label"
            currentIndex: Math.max(0, model.findIndex(s => s.id
                                                           === itemEditor.programView.store.selectedItem.template))

            onActivated: index => {
                itemEditor.programView.flushItem();
                itemEditor.programView.store.setTemplate(model[index].id);
            }
        }

        Text {
            visible: itemEditor.programView.store.selectedItem.kind === "song"
            text: "Arrangement"
            color: "#999999"
            font.pixelSize: 11
        }

        LibraryCombo {
            Layout.fillWidth: true
            visible: itemEditor.programView.store.selectedItem.kind === "song"
            model: itemEditor.programView.store.selectedItem.song?.arrangements || []
            textRole: "name"
            currentIndex: itemEditor.programView.store.selectedItem.song?.activeArrangement || 0

            onActivated: index => {
                itemEditor.programView.flushItem();
                itemEditor.programView.store.setArrangement(index);
                itemEditor.programView.previewIndex = 0;
            }
        }
    }

    RowLayout {
        visible: itemEditor.programView.store.layoutKind.length > 0

        Text {
            text: "Layout"
            color: "#999999"
            font.pixelSize: 11
        }

        Text {
            text: itemEditor.programView.store.selectedItem.layoutTemplate?.name
                  || "Automatic layout"
            color: "#BBBBBB"
            font.pixelSize: 11
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        StudioButton {
            text: "Choose template"

            onClicked: {
                itemEditor.programView.flushItem();
                itemEditor.programView.templatesRequested();
            }
        }

        StudioButton {
            text: "Reset"
            visible: !!itemEditor.programView.store.selectedItem.layoutTemplate?.id

            onClicked: {
                itemEditor.programView.flushItem();
                itemEditor.programView.store.clearLayout();
            }
        }
    }

    Text {
        text: "SPEAKER NOTES · THIS ITEM"
        font.pixelSize: 10
        font.letterSpacing: 1
        color: "#888888"
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 45
        clip: true

        TextArea {
            id: itemNotes

            objectName: "programItemNotes"
            enabled: itemEditor.programView.store.selected >= 0
            placeholderText: "Private notes for the speaker…"
            placeholderTextColor: "#777777"
            color: "#DDDDDD"
            font.pixelSize: 12
            padding: 10
            wrapMode: TextEdit.Wrap
            selectByMouse: true

            background: Rectangle {
                color: "#222222"
                border.color: itemNotes.activeFocus ? "#777777" : "#373737"
                radius: 6
            }

            onTextChanged: if (!itemEditor.programView.loading && activeFocus) {
                               itemEditor.programView.markItemDirty();
                           }
        }
    }
}
