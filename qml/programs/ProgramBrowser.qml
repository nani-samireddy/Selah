pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: browser

    required property var store
    readonly property var records: store.programs.filter(record => (record.title + " " + record.date).toLocaleLowerCase(
                                       ).includes(search.text.trim().toLocaleLowerCase()))

    signal programChosen(string programId)
    signal createRequested
    signal importRequested

    objectName: "programBrowser"
    spacing: 24

    RowLayout {
        Layout.fillWidth: true

        ColumnLayout {
            spacing: 6

            Text {
                text: "Programs"
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 23
                font.weight: Font.Medium
                color: LibraryStyle.text
            }

            Text {
                text: "Everything for your next service, in one place."
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 12
                color: LibraryStyle.muted
            }
        }

        Item {
            Layout.fillWidth: true
        }

        LibraryButton {
            text: "Import"

            onClicked: browser.importRequested()
        }

        LibraryButton {
            objectName: "createProgramButton"
            text: "New program"
            iconName: "plus"
            accented: true

            onClicked: browser.createRequested()
        }
    }

    RowLayout {
        Layout.fillWidth: true

        LibraryField {
            id: search

            objectName: "programSearch"
            leadingIcon: "search"
            placeholderText: "Search programs by name or date…"
            Layout.preferredWidth: 360
            Layout.maximumWidth: 440
            Layout.fillWidth: true
            Accessible.name: "Search programs"
        }

        LibraryButton {
            text: "Clear"
            quiet: true
            visible: search.text.length > 0

            onClicked: search.clear()
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            text: browser.records.length + (browser.records.length === 1 ? " program" : " programs")
            color: LibraryStyle.muted
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        GridView {
            id: grid

            objectName: "programCardGrid"
            anchors.fill: parent
            clip: true
            cellWidth: width / Math.max(1, Math.floor(width / 290))
            cellHeight: 186
            model: browser.records

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ProgramCard {
                required property var modelData

                objectName: "programCard_" + modelData.id
                width: grid.cellWidth - 16
                height: grid.cellHeight - 16
                record: modelData
                lastOpened: modelData.id === browser.store.id

                onClicked: browser.programChosen(modelData.id)
            }
        }

        Column {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 430)
            visible: browser.records.length === 0
            spacing: 10

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: search.text.trim() ? "No matching programs" : "Your first service starts here"
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 19
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: search.text.trim() ? "Try a different name or date." :
                                           "Create a program, then add songs, scripture, media and slides."
                color: LibraryStyle.muted
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 12
            }
        }
    }
}
