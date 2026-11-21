pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: orderPane

    required property var programView

    Layout.preferredWidth: 270
    Layout.maximumWidth: 290
    Layout.minimumWidth: 230
    Layout.fillHeight: true
    spacing: 10

    RowLayout {
        Text {
            text: "SERVICE ORDER"
            font.pixelSize: 10
            font.letterSpacing: 1
            color: LibraryStyle.tone("#888888")
            Layout.fillWidth: true
        }

        StudioButton {
            text: "Add item"
            iconName: "plus"
            enabled: orderPane.programView.store.id.length > 0

            onClicked: {
                orderPane.programView.flushItem();
                addMenu.popup();
            }
        }

        Menu {
            id: addMenu

            MenuItem {
                text: "Song from library"

                onTriggered: orderPane.programView.songsRequested()
            }

            MenuItem {
                text: "Bible passage"

                onTriggered: orderPane.programView.bibleRequested()
            }

            MenuItem {
                text: "Image or video"

                onTriggered: orderPane.programView.mediaRequested()
            }

            MenuSeparator {
            }

            MenuItem {
                text: "Current editor presentation"

                onTriggered: orderPane.programView.store.addPresentation(
                                 orderPane.programView.document.slides, "Custom presentation")
            }

            MenuItem {
                text: "Presentation file…"

                onTriggered: orderPane.programView.choosePresentationFile()
            }
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: orderList

            objectName: "programOrder"
            anchors.fill: parent
            spacing: 4
            clip: true
            cacheBuffer: orderDrag.dragging ? contentHeight : 300
            model: orderPane.programView.store.items

            ScrollBar.vertical: ScrollBar {
            }
            delegate: ItemDelegate {
                id: orderItem

                required property var modelData
                required property int index
                property bool isLive: modelData.id === orderPane.programView.store.liveItemId

                width: ListView.view.width - 8
                // Size rows to their two text lines, including Indian-script font metrics.
                height: Math.max(48, itemCopy.implicitHeight + topPadding + bottomPadding)
                leftPadding: 12
                rightPadding: 12
                topPadding: 8
                bottomPadding: 8
                opacity: orderDrag.dragging && orderDrag.sourceId === modelData.id ? 0.35 : 1
                Accessible.name: (orderItem.index + 1) + ". " + orderItem.modelData.title

                background: Rectangle {
                    color: orderItem.index === orderPane.programView.store.selected
                           ? LibraryStyle.tone("#303030") : orderItem.hovered ? LibraryStyle.tone(
                                                                                    "#242424") :
                                                                                LibraryStyle.tone(
                                                                                    "#202020")
                    radius: 7
                    border.color: orderItem.isLive ? "#7BA88D" : LibraryStyle.tone("#333333")
                }
                contentItem: Row {
                    spacing: 10

                    Item {
                        width: 28
                        height: parent.height

                        StudioIcon {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            name: "grip-vertical"
                            width: 11
                            height: 16
                            tone: reorderHandle.containsMouse ? LibraryStyle.tone("#CCCCCC") :
                                                                LibraryStyle.tone("#777777")
                        }

                        Text {
                            x: 13
                            y: 2
                            text: (orderItem.index + 1).toString().padStart(2, "0")
                            color: orderItem.isLive ? "#ABD3BC" : LibraryStyle.tone("#777777")
                            font.pixelSize: 10
                        }

                        MouseArea {
                            id: reorderHandle

                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            ToolTip.visible: containsMouse && !pressed
                            ToolTip.text: "Drag to reorder"
                            ToolTip.delay: 500

                            onPressed: mouse => orderDrag.arm(orderItem.index, mapToItem(orderList,
                                                                                         mouse.x, mouse.y))
                            onPositionChanged: mouse => {
                                if (pressed)
                                    orderDrag.update(mapToItem(orderList, mouse.x, mouse.y));
                            }
                            onReleased: {
                                if (orderDrag.sourceId.length && !orderDrag.finish())
                                    orderPane.programView.select(orderItem.index);
                            }
                            onCanceled: orderDrag.cancel()
                        }
                    }

                    Column {
                        id: itemCopy

                        width: parent.width - 38
                        spacing: 4

                        Text {
                            width: parent.width
                            text: orderItem.modelData.title
                            color: LibraryStyle.tone("#DDDDDD")
                            font.pixelSize: 13
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: (orderItem.isLive ? "LIVE · " : "") + orderItem.modelData.kind + " · "
                                  + orderItem.modelData.slides.length + (
                                      orderItem.modelData.slides.length === 1 ? " slide" :
                                                                                " slides")
                            color: orderItem.isLive ? "#ABD3BC" : LibraryStyle.tone("#888888")
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                }

                onClicked: orderPane.programView.select(index)
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 20
                visible: orderPane.programView.store.items.length === 0
                text: orderPane.programView.store.id
                      ? "Add songs, passages, media\nand presentations to your service." :
                        "Create your first program\nto prepare a complete service."
                color: LibraryStyle.tone("#888888")
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }

        ProgramOrderDrag {
            id: orderDrag

            anchors.fill: parent
            z: 20
            view: orderList
            programView: orderPane.programView
        }
    }

    RowLayout {
        Layout.fillWidth: true

        StudioButton {
            text: "Up"
            enabled: orderPane.programView.store.selected > 0
            Layout.fillWidth: true

            onClicked: {
                orderPane.programView.flushItem();
                orderPane.programView.store.moveItem(-1);
            }
        }

        StudioButton {
            text: "Down"
            enabled: orderPane.programView.store.selected >= 0
                     && orderPane.programView.store.selected
                     < orderPane.programView.store.items.length - 1
            Layout.fillWidth: true

            onClicked: {
                orderPane.programView.flushItem();
                orderPane.programView.store.moveItem(1);
            }
        }

        StudioButton {
            text: "Remove"
            enabled: orderPane.programView.store.selected >= 0
            Layout.fillWidth: true

            onClicked: {
                orderPane.programView.flushItem();
                orderPane.programView.store.removeSelected();
            }
        }
    }
}
