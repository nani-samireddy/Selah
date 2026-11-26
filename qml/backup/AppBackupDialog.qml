pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: dialog

    required property var controller
    property url chosenBackup: ""

    signal quitRequested

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(600, parent.width - 40)
    padding: 24
    modal: true
    closePolicy: controller.busy ? Popup.NoAutoClose : Popup.CloseOnEscape

    background: SquircleSurface {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 16
    }
    contentItem: ColumnLayout {
        spacing: 18

        RowLayout {
            Text {
                text: "Backup & restore"
                color: LibraryStyle.text
                font.pixelSize: 22
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Close"
                enabled: !dialog.controller.busy

                onClicked: dialog.close()
            }
        }

        Text {
            text: "Keep a full copy of your songs, Bibles, programs, presentations, templates, media, downloaded fonts and preferences."
            color: LibraryStyle.muted
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        LibraryButton {
            text: "Export full backup…"
            accented: true
            enabled: !dialog.controller.busy && !dialog.controller.restoreReady

            onClicked: exportFile.open()
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: LibraryStyle.border
        }

        Text {
            text: "Restore a full backup"
            font.weight: Font.Medium
            color: LibraryStyle.text
        }

        Text {
            text: "Restoring replaces all app data on the next launch. A copy of your current data is kept for rollback. Audience, stage and NDI outputs start off."
            color: LibraryStyle.muted
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        LibraryButton {
            text: "Choose backup…"
            enabled: !dialog.controller.busy && !dialog.controller.restoreReady

            onClicked: importFile.open()
        }

        Text {
            text: dialog.chosenBackup.toString()
            visible: text.length > 0
            elide: Text.ElideMiddle
            Layout.fillWidth: true
            color: LibraryStyle.muted
        }

        LibraryButton {
            text: "Prepare restore"
            visible: dialog.chosenBackup.toString().length > 0 && !dialog.controller.restoreReady
            enabled: !dialog.controller.busy

            onClicked: dialog.controller.importBackup(dialog.chosenBackup)
        }

        ProgressBar {
            visible: dialog.controller.busy
            Layout.fillWidth: true
            from: 0
            to: 100
            value: dialog.controller.progress
            indeterminate: value === 0
        }

        Text {
            text: dialog.controller.status
            visible: text.length > 0
            color: LibraryStyle.text
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        RowLayout {
            LibraryButton {
                text: "Cancel"
                visible: dialog.controller.busy

                onClicked: dialog.controller.cancel()
            }

            LibraryButton {
                text: "Cancel restore"
                visible: dialog.controller.restoreReady

                onClicked: dialog.controller.cancelRestore()
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Quit to restore"
                accented: true
                visible: dialog.controller.restoreReady

                onClicked: dialog.quitRequested()
            }
        }
    }

    FileDialog {
        id: exportFile

        title: "Export full Selah backup"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "selahbackup"
        nameFilters: ["Selah full backups (*.selahbackup)"]

        onAccepted: dialog.controller.exportBackup(selectedFile)
    }

    FileDialog {
        id: importFile

        title: "Choose a full Selah backup"
        nameFilters: ["Selah full backups (*.selahbackup)"]

        onAccepted: dialog.chosenBackup = selectedFile
    }
}
