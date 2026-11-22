pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: rail

    required property var controller

    signal chosen(int index)
    signal prepareRequested

    color: LibraryStyle.panel
    radius: 10
    border.color: LibraryStyle.divider

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            StudioIcon {
                name: "list-ordered"
            }

            Text {
                text: "Service slides"
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 13
                Layout.fillWidth: true
            }

            Text {
                text: rail.controller.liveCount
                color: LibraryStyle.muted
                font.pixelSize: 11
            }
        }

        ListView {
            id: order

            objectName: "liveServiceOrder"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: rail.controller.liveOrder
            currentIndex: rail.controller.liveIndex

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: row

                required property var modelData
                required property int index
                readonly property bool current: index === rail.controller.liveIndex

                width: ListView.view.width - 6
                height: 54
                padding: 9
                Accessible.name: "Present slide " + (index + 1) + ": " + modelData.title

                background: Rectangle {
                    radius: 7
                    color: row.current ? LibraryStyle.tone("#333333") : row.hovered
                                         ? LibraryStyle.hover : "transparent"
                    border.color: row.current ? LibraryStyle.tone("#606060") : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 9

                    Text {
                        text: String(row.index + 1).padStart(2, "0")
                        color: row.current ? LibraryStyle.text : LibraryStyle.muted
                        font.pixelSize: 10
                        Layout.preferredWidth: 20
                    }

                    ColumnLayout {
                        spacing: 4

                        Text {
                            text: row.modelData.title
                            color: LibraryStyle.text
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: row.modelData.itemTitle || "Presentation"
                            color: LibraryStyle.muted
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                onClicked: rail.chosen(index)
            }

            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                visible: !rail.controller.liveCount
                text: "No program is live.\nChoose a program to begin."
                color: LibraryStyle.muted
                font.pixelSize: 12
                lineHeight: 1.5
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }

        StudioButton {
            text: "Programs"
            iconName: "list-ordered"
            Layout.fillWidth: true

            onClicked: rail.prepareRequested()
        }
    }
}
