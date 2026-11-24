pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: itemEditor

    required property var programView
    readonly property string titleText: programView.store.selectedItem.title || ""
    property alias notesText: itemSettings.notesText

    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 8

    RowLayout {
        Text {
            text: "PREVIEW"
            font.pixelSize: 10
            font.letterSpacing: 1
            color: LibraryStyle.tone("#888888")
            Layout.fillWidth: true
        }

        Text {
            text: itemEditor.programView.store.selectedSlides.length ? (
                                                                           itemEditor.programView.previewIndex
                                                                           + 1) + " / "
                                                                       + itemEditor.programView.store.selectedSlides.length :
                                                                       ""
            font.pixelSize: 11
            color: LibraryStyle.tone("#999999")
        }
    }

    Rectangle {
        id: draftFrame

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: width * 9 / 16
        Layout.maximumHeight: itemEditor.programView.workspaceMode ? 420 : 285
        Layout.minimumHeight: 100
        color: LibraryStyle.tone("#111111")
        border.color: LibraryStyle.tone("#363636")
        radius: 5
        clip: true

        SlideRenderer {
            showTransparencyGrid: true
            width: 1920
            height: 1080
            scale: Math.min(draftFrame.width / 1920, draftFrame.height / 1080)
            transformOrigin: Item.TopLeft
            x: (draftFrame.width - width * scale) / 2
            y: (draftFrame.height - height * scale) / 2
            elements: itemEditor.programView.previewSlide.elements || []
            backgroundFill: itemEditor.programView.previewSlide.backgroundFill || ({})
            backgroundColor: itemEditor.programView.previewSlide.background || LibraryStyle.tone(
                                 "#111111")
            ambientOpacity: itemEditor.programView.previewSlide.ambientOpacity === undefined ? 0.5 :
                                                                                               itemEditor.programView.previewSlide.ambientOpacity
            videoEnabled: false
            playing: false
        }

        Text {
            anchors.centerIn: parent
            visible: !itemEditor.programView.store.selectedSlides.length
            text: "Select a service item"
            color: LibraryStyle.tone("#777777")
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
            color: LibraryStyle.tone("#111111")
            radius: 5
            border.width: 2
            border.color: index === itemEditor.programView.previewIndex ? LibraryStyle.tone(
                                                                              "#AAAAAA") :
                                                                          LibraryStyle.tone(
                                                                              "#383838")
            clip: true

            SlideRenderer {
                showTransparencyGrid: true
                x: 3
                y: 3
                width: 1920
                height: 1080
                scale: 106 / 1920
                transformOrigin: Item.TopLeft
                elements: slideThumb.modelData.elements || []
                backgroundFill: slideThumb.modelData.backgroundFill || ({})
                backgroundColor: slideThumb.modelData.background
                ambientOpacity: slideThumb.modelData.ambientOpacity === undefined ? 0.5 :
                                                                                    slideThumb.modelData.ambientOpacity
                videoEnabled: false
            }

            MouseArea {
                anchors.fill: parent

                onClicked: itemEditor.programView.previewIndex = slideThumb.index
            }
        }
    }

    ProgramItemSettings {
        id: itemSettings

        programView: itemEditor.programView
    }

    Item {
        Layout.fillHeight: true
    }
}
