pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: rail

    required property var controller
    property var expanded: ({})
    readonly property var groups: {
        let result = [];
        for (const slide of controller.liveOrder) {
            const key = slide.itemId || "presentation";
            let group = result[result.length - 1];
            if (!group || group.id !== key) {
                group = {
                    id: key,
                    title: slide.itemTitle || "Presentation",
                    slides: []
                };
                result.push(group);
            }
            group.slides.push(slide);
        }
        return result;
    }
    readonly property int activeSlide: controller.liveIndex

    signal chosen(int index)
    signal prepareRequested

    function revealCurrent() {
        const group = groups.find(group => group.slides.some(slide => slide.index
                                                                      === controller.liveIndex));
        if (group) {
            let next = Object.assign({}, expanded);
            next[group.id] = true;
            expanded = next;
            Qt.callLater(scrollToCurrent);
        }
    }

    function scrollToCurrent() {
        const groupIndex = groups.findIndex(group => group.slides.some(slide => slide.index
                                                                                === controller.liveIndex));
        if (groupIndex < 0)
            return;
        order.positionViewAtIndex(groupIndex, ListView.Contain);
        const row = order.itemAtIndex(groupIndex);
        if (!row)
            return;
        const childIndex = groups[groupIndex].slides.findIndex(slide => slide.index
                                                                        === controller.liveIndex);
        const top = row.y + 49 + childIndex * 43;
        if (top < order.contentY)
            order.contentY = top;
        else if (top + 40 > order.contentY + order.height)
            order.contentY = top + 40 - order.height;
    }

    objectName: "presentServiceRail"
    color: LibraryStyle.panel
    radius: 10
    border.color: LibraryStyle.divider

    onGroupsChanged: revealCurrent()
    onActiveSlideChanged: revealCurrent()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            Text {
                text: "Service order"
                color: LibraryStyle.text
                font.pixelSize: 13
                Layout.fillWidth: true
            }

            Text {
                text: rail.groups.length
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
            spacing: 6
            model: rail.groups
            currentIndex: rail.groups.findIndex(group => group.slides.some(slide => slide.index
                                                                                    === rail.controller.liveIndex))

            ScrollBar.vertical: ScrollBar {
            }
            delegate: Column {
                id: groupRow

                required property var modelData
                readonly property bool opened: !!rail.expanded[modelData.id]
                readonly property bool containsLive: modelData.slides.some(slide => slide.index
                                                                                    === rail.controller.liveIndex)

                objectName: "liveServiceGroup_" + modelData.id
                width: order.width - 6
                spacing: 3

                ItemDelegate {
                    objectName: "liveServiceToggle_" + groupRow.modelData.id
                    width: groupRow.width
                    height: 46
                    padding: 8
                    Accessible.name: (groupRow.opened ? "Collapse " : "Expand ")
                                     + groupRow.modelData.title

                    background: Rectangle {
                        radius: 6
                        color: groupRow.containsLive ? LibraryStyle.hover : LibraryStyle.field
                    }
                    contentItem: RowLayout {
                        spacing: 6

                        StudioIcon {
                            name: groupRow.opened ? "chevron-down" : "chevron-right"
                        }

                        Text {
                            text: groupRow.modelData.title
                            color: LibraryStyle.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: groupRow.modelData.slides.length
                            color: LibraryStyle.muted
                            font.pixelSize: 10
                        }
                    }

                    onClicked: {
                        let next = Object.assign({}, rail.expanded);
                        next[groupRow.modelData.id] = !groupRow.opened;
                        rail.expanded = next;
                    }
                }

                Repeater {
                    model: groupRow.opened ? groupRow.modelData.slides : []

                    ItemDelegate {
                        id: child

                        required property var modelData
                        required property int index
                        readonly property bool current: modelData.index
                                                        === rail.controller.liveIndex

                        objectName: "liveServiceSlide_" + modelData.index
                        width: groupRow.width - 16
                        x: 16
                        height: 40
                        padding: 8
                        Accessible.name: "Present slide " + (modelData.index + 1) + ": "
                                         + modelData.title

                        background: Rectangle {
                            radius: 5
                            color: child.current ? LibraryStyle.hover : child.hovered
                                                   ? LibraryStyle.field : "transparent"
                            border.color: child.current ? LibraryStyle.focus : "transparent"
                        }
                        contentItem: RowLayout {
                            Text {
                                text: String(child.index + 1).padStart(2, "0")
                                color: LibraryStyle.muted
                                font.pixelSize: 10
                                Layout.preferredWidth: 20
                            }

                            Text {
                                text: child.modelData.title
                                color: LibraryStyle.text
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        onClicked: rail.chosen(modelData.index)
                    }
                }
            }

            onCurrentIndexChanged: Qt.callLater(() => positionViewAtIndex(currentIndex,
                                                                          ListView.Contain))

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                visible: !rail.controller.liveCount
                text: "No program is loaded.\nChoose a program to begin."
                color: LibraryStyle.muted
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }

        LibraryButton {
            text: "Programs"
            iconName: "list-ordered"
            Layout.fillWidth: true

            onClicked: rail.prepareRequested()
        }
    }
}
