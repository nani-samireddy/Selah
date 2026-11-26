pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var controller
    property bool presenting: false

    objectName: "updateDialog"
    title: "Selah updates"
    header: null
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(620, parent.width - 40)
    height: Math.min(controller.release.notes ? 560 : 420, parent.height - 40)
    padding: 24
    standardButtons: Dialog.NoButton

    background: SquircleSurface {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 12
    }
    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            Text {
                text: "Selah updates"
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 22
                color: LibraryStyle.text
                Layout.fillWidth: true
            }

            LibraryButton {
                iconName: "x"
                hint: "Close updates"
                quiet: true

                onClicked: dialog.close()
            }
        }

        LibraryCaption {
            text: "Installed: " + dialog.controller.installedVersion + " · " + (
                      dialog.controller.platform || "Unsupported platform") + " · Stable channel"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        LibraryCaption {
            objectName: "updateStatus"
            text: dialog.controller.status
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        ProgressBar {
            visible: dialog.controller.busy
            Layout.fillWidth: true
            indeterminate: dialog.controller.total <= 0
            from: 0
            to: Math.max(1, dialog.controller.total)
            value: dialog.controller.received
        }

        LibraryCaption {
            visible: dialog.controller.busy && dialog.controller.total > 0
            text: (dialog.controller.received / 1048576).toFixed(1) + " / " + (
                      dialog.controller.total / 1048576).toFixed(1) + " MB"
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            Text {
                width: parent.width
                text: dialog.controller.release.notes || "Published release notes will appear here."
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 13
                lineHeight: 1.3
            }
        }

        LibraryCaption {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: dialog.controller.downloadedPath ? (dialog.presenting
                                                      ? "Stop audience, stage and NDI outputs before installing. " :
                                                        "") + (Qt.platform.os === "osx"
                                                               ? "Open the downloaded disk image or ZIP, quit Selah, replace the app in Applications, then reopen it." :
                                                                 Qt.platform.os === "windows"
                                                                 ? "Extract the ZIP to a new folder, quit Selah, then run Selah.cmd from that folder. Keep your previous copy until the new one works." :
                                                                   "Extract the archive to a new folder, quit Selah, then run the included launcher.") :
                                                     "Updates are downloaded only when you choose. Installation is manual for this release; your saved library stays in the app-data folder."
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryButton {
                text: dialog.controller.busy ? "Cancel" : "Check again"

                onClicked: dialog.controller.busy ? dialog.controller.cancel() :
                                                    dialog.controller.check()
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Later"
                quiet: true

                onClicked: dialog.close()
            }

            LibraryButton {
                objectName: "downloadUpdateButton"
                visible: !dialog.controller.downloadedPath
                text: "Download update"
                accented: true
                enabled: !dialog.controller.busy && !!dialog.controller.release.url

                onClicked: dialog.controller.download()
            }

            LibraryButton {
                visible: !!dialog.controller.downloadedPath
                text: "Open download folder"
                accented: true
                enabled: !dialog.presenting && !dialog.controller.busy

                onClicked: dialog.controller.showDownload()
            }
        }
    }
}
