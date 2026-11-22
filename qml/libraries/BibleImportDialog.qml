pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog

    required property var library
    property url selectedUrl: ""
    readonly property var records: library.repositoryBibles.filter(record => (record.title + " "
                                                                             + record.language).toLowerCase(
                                       ).includes(search.text.trim().toLowerCase()))

    objectName: "bibleImportDialog"
    title: "Import Bible translation"
    modal: true
    width: Math.min(650, parent.width - 48)
    height: Math.min(620, parent.height - 48)
    anchors.centerIn: parent
    padding: 20

    background: Rectangle {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 12
    }
    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            Text {
                text: "Holy Bible XML repository"
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 15
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Import local file"
                iconName: "upload"
                enabled: !dialog.library.busy

                onClicked: localFile.open()
            }
        }

        Text {
            text: "Choose a translation to download and keep in your offline library."
            color: LibraryStyle.muted
            font.pixelSize: 11
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        LibraryField {
            id: search

            objectName: "repositoryBibleSearch"
            placeholderText: "Search languages or translations…"
            Layout.fillWidth: true
        }

        ListView {
            id: translations

            objectName: "repositoryBibleList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: dialog.records

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: row

                required property var modelData

                objectName: "repositoryBible_" + modelData.filename
                width: ListView.view.width - 8
                height: 57
                padding: 10
                enabled: !dialog.library.busy
                Accessible.name: modelData.title

                background: Rectangle {
                    radius: 7
                    color: dialog.selectedUrl.toString() === row.modelData.url.toString()
                           ? LibraryStyle.hover : row.hovered ? LibraryStyle.field : "transparent"
                    border.color: dialog.selectedUrl.toString() === row.modelData.url.toString()
                                  ? LibraryStyle.focus : "transparent"
                }
                contentItem: ColumnLayout {
                    spacing: 5

                    Text {
                        text: row.modelData.title
                        color: LibraryStyle.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: row.modelData.language + " · " + (row.modelData.bytes / 1048576).toFixed(
                                  1) + " MB"
                        color: LibraryStyle.muted
                        font.pixelSize: 10
                    }
                }

                onClicked: {
                    dialog.selectedUrl = modelData.url;
                    link.text = modelData.url.toString();
                }
            }

            Text {
                anchors.centerIn: parent
                visible: dialog.records.length === 0
                text: "No matching translations"
                color: LibraryStyle.muted
                font.pixelSize: 12
            }
        }

        LibraryField {
            id: link

            Layout.fillWidth: true
            placeholderText: "Or paste a GitHub / raw XML link from this repository…"
            enabled: !dialog.library.busy

            onTextEdited: dialog.selectedUrl = text.trim()
        }

        Text {
            visible: !dialog.library.busy
            text: dialog.library.busy || dialog.library.status !== "Library ready" ? dialog.library.status :
                                                                                     "Source attribution is retained with each imported translation."
            color: LibraryStyle.muted
            font.pixelSize: 11
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        BibleImportProgress {
            library: dialog.library
            Layout.fillWidth: true
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Close"

                onClicked: dialog.close()
            }

            StudioButton {
                objectName: "importBibleDownloadButton"
                text: dialog.library.busy ? "Importing…" : "Download & import"
                accented: true
                enabled: !dialog.library.busy && dialog.selectedUrl.toString().length > 0

                onClicked: dialog.library.importBible(dialog.selectedUrl)
            }
        }
    }

    FileDialog {
        id: localFile

        title: "Import Bible JSON or XML"
        nameFilters: ["Bible translations (*.json *.xml)", "Bible XML (*.xml)",
            "Selah Bible packages (*.json)"]

        onAccepted: dialog.library.importBible(selectedFile)
    }

    Connections {
        function onBibleImported(translationId) {
            dialog.close();
        }

        target: dialog.library
    }
}
