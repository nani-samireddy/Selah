pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: bar

    required property var host

    implicitHeight: 54
    color: LibraryStyle.tone("#151515")

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: LibraryStyle.tone("#2C2C2C")
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 10

        StudioButton {
            objectName: "editorLibraryNavigation"
            text: bar.host.templateMode ? "Templates" : "Presentations"
            iconName: "chevron-left"
            quiet: true
            hint: "Return to library"

            onClicked: bar.host.browseLibrary(bar.host.templateMode ? "templates" : "custom")
        }

        Column {
            Layout.fillWidth: true
            Layout.preferredWidth: 0
            Layout.minimumWidth: 0
            spacing: 3

            Text {
                width: parent.width
                elide: Text.ElideRight
                text: bar.host.templateMode ? "Template design" : bar.host.programEditingItem
                                              ? "Program item design" :
                                                bar.host.customPresentationTitle || "Presentation"
                color: LibraryStyle.tone("#DEDEDE")
                font.pixelSize: 12
            }

            Text {
                width: parent.width
                elide: Text.ElideRight
                text: bar.host.doc.importing ? "Importing media…" : bar.host.doc.status
                color: LibraryStyle.tone("#858585")
                font.pixelSize: 10
            }
        }

        StudioButton {
            text: "Update program item"
            visible: bar.host.programEditingItem.length > 0
            enabled: bar.host.programEditingItem === bar.host.programs.selectedItem.id

            onClicked: {
                if (bar.host.programs.updateSelectedSlides(bar.host.doc.slides)) {
                    bar.host.notify("Program item updated");
                    bar.host.showProgramEditor();
                }
            }
        }

        StudioButton {
            id: fileActions

            objectName: "editorFileActionsButton"
            text: "File"
            iconName: "chevron-down"
            quiet: true
            hint: "Open or export a presentation"

            onClicked: fileMenu.open()

            Menu {
                id: fileMenu

                objectName: "editorFileMenu"
                y: fileActions.height + 6
                popupType: Popup.Item
                implicitWidth: 220
                padding: 5

                background: SquircleSurface {
                    radius: 9
                    color: LibraryStyle.panel
                    border.color: LibraryStyle.border
                }

                FileAction {
                    text: "Open presentation…"
                    glyph: "download"

                    onTriggered: bar.host.openPresentation()
                }

                FileAction {
                    text: "Export presentation…"
                    glyph: "upload"

                    onTriggered: bar.host.exportPresentation()
                }
            }
        }

        StudioButton {
            visible: !bar.host.templateMode
            text: "Save to library"
            hint: bar.host.editingPresentationId ? "Update the saved presentation" :
                                                   "Save this presentation to the library"

            onClicked: bar.host.saveCustomPresentation()
        }

        StudioButton {
            objectName: "editorPreviewButton"
            iconName: bar.host.editorPreviewOpen ? "x" : "play"
            hint: bar.host.editorPreviewOpen ? "Close slide preview" : "Preview the current slide"
            selected: bar.host.editorPreviewOpen
            quiet: true
            implicitWidth: 32

            onClicked: bar.host.editorPreviewOpen ? bar.host.closeEditorPreview() : bar.host.preview(
                                                        )
        }

        StudioButton {
            text: bar.host.templateMode ? "Save template" : "Present slide"
            accented: true

            onClicked: {
                if (bar.host.templateMode)
                    bar.host.saveCurrentTemplate();
                else {
                    bar.host.outputs.takeLive(bar.host.doc.slides, bar.host.doc.currentSlide);
                    bar.host.switchWorkspace("present");
                }
            }
        }
    }

    component FileAction: MenuItem {
        id: fileChoice

        required property string glyph

        implicitHeight: 32

        contentItem: RowLayout {
            spacing: 10

            StudioIcon {
                name: fileChoice.glyph
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
            }

            Text {
                text: fileChoice.text
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 12
                Layout.fillWidth: true
            }
        }
        background: SquircleSurface {
            radius: 5
            color: fileChoice.highlighted ? LibraryStyle.hover : "transparent"
        }
    }
}
