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
            color: "#888888"
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

    ListView {
        id: orderList

        objectName: "programOrder"
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 6
        clip: true
        model: orderPane.programView.store.items

        ScrollBar.vertical: ScrollBar {
        }
        delegate: ItemDelegate {
            id: orderItem

            required property var modelData
            required property int index
            property bool isLive: modelData.id === orderPane.programView.store.liveItemId

            width: ListView.view.width - 8
            height: 78
            Accessible.name: (orderItem.index + 1) + ". " + orderItem.modelData.title

            background: Rectangle {
                color: orderItem.index === orderPane.programView.store.selected ? "#303030" :
                                                                                  orderItem.hovered
                                                                                  ? "#242424" :
                                                                                    "#202020"
                radius: 7
                border.color: orderItem.isLive ? "#7BA88D" : "#333333"
            }
            contentItem: Row {
                spacing: 10

                Text {
                    width: 22
                    text: (orderItem.index + 1).toString().padStart(2, "0")
                    color: orderItem.isLive ? "#ABD3BC" : "#777777"
                    font.pixelSize: 11
                    topPadding: 3
                }

                Column {
                    width: parent.width - 32
                    spacing: 7

                    Text {
                        width: parent.width
                        text: orderItem.modelData.title
                        color: "#DDDDDD"
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: (orderItem.isLive ? "LIVE · " : "") + orderItem.modelData.kind
                              + " · " + orderItem.modelData.slides.length + " slides"
                        color: orderItem.isLive ? "#ABD3BC" : "#888888"
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
            color: "#888888"
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
            wrapMode: Text.Wrap
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
