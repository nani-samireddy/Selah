pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

RowLayout {
    id: editorBar

    required property var programView
    readonly property bool dialogOpen: details.opened

    signal backRequested

    spacing: 16
    Layout.fillWidth: true

    LibraryButton {
        objectName: "allProgramsButton"
        text: "All programs"
        iconName: "chevron-left"
        quiet: true

        onClicked: editorBar.backRequested()
    }

    Rectangle {
        implicitWidth: 1
        implicitHeight: 30
        color: LibraryStyle.divider
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 4

        Text {
            objectName: "programHeading"
            Layout.fillWidth: true
            text: editorBar.programView.store.title
            color: LibraryStyle.text
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 21
            font.weight: Font.Medium
            elide: Text.ElideRight
        }

        Text {
            text: editorBar.programView.store.date || "No service date"
            color: LibraryStyle.muted
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
        }
    }

    LibraryButton {
        objectName: "editProgramDetailsButton"
        text: "Edit details"
        iconName: "pen-tool"
        quiet: true

        onClicked: {
            editorBar.programView.flushItem();
            details.open();
        }
    }

    LibraryButton {
        iconName: "undo-2"
        hint: "Undo program change"
        enabled: editorBar.programView.store.canUndo

        onClicked: {
            editorBar.programView.flushItem();
            editorBar.programView.store.undo();
        }
    }

    LibraryButton {
        iconName: "redo-2"
        hint: "Redo program change"
        enabled: editorBar.programView.store.canRedo

        onClicked: {
            editorBar.programView.flushItem();
            editorBar.programView.store.redo();
        }
    }

    LibraryButton {
        text: "Export"

        onClicked: editorBar.programView.showExportDialog()
    }

    LibraryButton {
        objectName: "presentProgramButton"
        text: editorBar.programView.workspaceMode ? "Present from here" : "Take slide live"
        iconName: "play"
        accented: true
        enabled: editorBar.programView.store.selectedSlides.length > 0
        hint: "Present from the selected preview slide"

        onClicked: editorBar.programView.takeLive()
    }

    ProgramDetailsDialog {
        id: details

        store: editorBar.programView.store
    }
}
