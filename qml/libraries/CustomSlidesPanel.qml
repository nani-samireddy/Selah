pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    required property var library
    property var programTarget: null
    property bool workspaceMode: false
    readonly property var results: library.presentations.filter(item => item.title.toLocaleLowerCase(
                                                                            ).includes(
                                                                            search.text.trim(
                                                                                ).toLocaleLowerCase(
                                                                                )))
    property string message: ""

    signal editRequested(var presentation)
    signal createRequested

    function addToProgram(definition) {
        return addToDestination(definition, "");
    }

    function addToDestination(definition, destination) {
        if (programTarget && (destination || programTarget.id) && programTarget.addPresentation(
                    definition.slides, definition.title, destination)) {
            message = "Added to " + ((programTarget.programs.find(program => program.id === (
                                                                                 destination
                                                                                 || programTarget.id))
                                      || {}).title || programTarget.title);
            return true;
        }
        message = programTarget ? programTarget.status : "Open a program first";
        return false;
    }

    objectName: "customSlidesPanel"
    width: parent.width
    height: parent.height
    padding: LibraryStyle.pagePadding
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: LibraryStyle.panel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Column {
                spacing: 5

                Text {
                    text: "Custom slides"
                    color: LibraryStyle.text
                    font.pixelSize: 22
                    font.family: LibraryStyle.fontFamily
                }

                Text {
                    text: "Your saved presentations. Open in Design or add a copy to a program."
                    color: LibraryStyle.muted
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                objectName: "newCustomPresentation"
                text: "New presentation"
                iconName: "plus"

                onClicked: panel.createRequested()
            }
        }

        LibraryField {
            id: search

            Layout.fillWidth: true
            leadingIcon: "search"
            placeholderText: "Search custom presentations…"
        }

        GridView {
            id: cards

            objectName: "customPresentationsGrid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.results
            cellWidth: width / Math.max(1, Math.floor(width / 280))
            cellHeight: cellWidth * 9 / 16 + 132

            ScrollBar.vertical: ScrollBar {
            }
            delegate: Rectangle {
                id: card

                required property var modelData

                width: cards.cellWidth - 12
                height: cards.cellHeight - 12
                radius: 8
                color: LibraryStyle.field
                border.color: LibraryStyle.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Item {
                        id: preview

                        readonly property var firstSlide: card.modelData.slides[0] || ({})

                        Layout.fillWidth: true
                        Layout.preferredHeight: width * 9 / 16
                        clip: true

                        SlideRenderer {
                            width: 1920
                            height: 1080
                            scale: preview.width / 1920
                            transformOrigin: Item.TopLeft
                            elements: preview.firstSlide.elements || []
                            backgroundColor: preview.firstSlide.background || "#171717"
                            ambientOpacity: preview.firstSlide.ambientOpacity === undefined ? 0.85 :
                                                                                              preview.firstSlide.ambientOpacity
                            videoEnabled: false
                            audible: false
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: card.modelData.title
                        textFormat: Text.PlainText
                        color: LibraryStyle.text
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: card.modelData.slides.length + " slides"
                            color: LibraryStyle.muted
                            font.pixelSize: 11
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        LibraryButton {
                            text: "Edit"
                            iconName: "pen-tool"

                            onClicked: panel.editRequested(card.modelData)
                        }
                    }

                    ProgramAddButton {
                        objectName: "customProgramDestination_" + card.modelData.id
                        Layout.fillWidth: true
                        store: panel.programTarget

                        onAddRequested: programId => panel.addToDestination(card.modelData,
                                                                            programId)
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: cards.count === 0
                text: search.text ? "No matching presentations" :
                                    "Create your first custom presentation.\nIt will open in Design."
                color: LibraryStyle.muted
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Text {
            text: panel.message || panel.library.status
            color: LibraryStyle.muted
            font.pixelSize: 11
        }
    }
}
