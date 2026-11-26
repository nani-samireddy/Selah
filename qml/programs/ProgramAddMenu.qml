pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Menu {
    id: programMenu

    required property var programView

    objectName: "programAddMenu"
    popupType: Popup.Item
    implicitWidth: 252
    padding: 6

    background: Rectangle {
        radius: 9
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
    }

    MenuItem {
        enabled: false
        implicitHeight: 24

        contentItem: StudioMicroLabel {
            text: "FROM LIBRARY"
        }
    }

    AddChoice {
        text: "Songs"
        glyph: "type"

        onTriggered: programMenu.programView.songsRequested()
    }

    AddChoice {
        text: "Bible passages"
        glyph: "book-open"

        onTriggered: programMenu.programView.bibleRequested()
    }

    AddChoice {
        text: "Media"
        glyph: "image"

        onTriggered: programMenu.programView.mediaRequested()
    }

    AddChoice {
        objectName: "addSavedPresentation"
        text: "Presentations"
        glyph: "layers"

        onTriggered: programMenu.programView.presentationsRequested()
    }

    MenuSeparator {
        topPadding: 5
        bottomPadding: 5

        contentItem: Rectangle {
            implicitHeight: 1
            color: LibraryStyle.divider
        }
    }

    AddChoice {
        objectName: "copyEditorPresentation"
        text: "Copy editor slides"
        glyph: "copy"
        enabled: !programMenu.programView.operatorWindow.templateMode
                 && programMenu.programView.document.slides.length > 0

        onTriggered: {
            const host = programMenu.programView.operatorWindow;
            if (host.activeTextEditor)
                host.activeTextEditor.finish();
            programMenu.programView.store.addPresentation(programMenu.programView.document.slides,
                                                          host.customPresentationTitle
                                                          || "Presentation");
        }
    }

    AddChoice {
        text: "Import Selah presentation…"
        glyph: "download"

        onTriggered: programMenu.programView.choosePresentationFile()
    }

    component AddChoice: MenuItem {
        id: choice

        property string glyph: ""

        implicitHeight: 36
        leftPadding: 10
        rightPadding: 10

        contentItem: RowLayout {
            spacing: 10

            StudioIcon {
                name: choice.glyph
                opacity: choice.enabled ? 1 : 0.4
            }

            Text {
                text: choice.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 13
                color: choice.enabled ? LibraryStyle.text : LibraryStyle.muted
                Layout.fillWidth: true
            }
        }
        background: Rectangle {
            radius: 5
            color: choice.highlighted ? LibraryStyle.hover : "transparent"
        }
    }
}
