pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SquircleSurface {
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
                    section: slide.section || "",
                    title: slide.itemTitle || "Presentation",
                    quickAddition: !!slide.quickAddition,
                    slides: []
                };
                result.push(group);
            }
            group.slides.push(slide);
        }
        return result;
    }
    readonly property int activeSlide: controller.liveIndex
    readonly property color currentFill: LibraryStyle.dark ? "#23352C" : "#DFEDE4"
    readonly property color currentAccent: LibraryStyle.dark ? "#A6D0B7" : "#316A49"

    signal chosen(int index)
    signal prepareRequested

    function slideLabel(slide, group) {
        const title = slide.title || "";
        const prefix = group.title + " · ";
        if (title.startsWith(prefix))
            return title.slice(prefix.length);
        return title === group.title || !title.length ? "Slide " + (group.slides.findIndex(item
                                                                                           => item.index
                                                                                              === slide.index)
                                                                    + 1) : title;
    }

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
        let child = null;
        for (const item of row.children) {
            if (item.objectName === "liveServiceSlide_" + controller.liveIndex) {
                child = item;
                break;
            }
        }
        const top = child ? child.mapToItem(order.contentItem, 0, 0).y : row.y;
        const bottom = top + (child ? child.height : row.height);
        if (top < order.contentY)
            order.contentY = top;
        else if (bottom > order.contentY + order.height)
            order.contentY = bottom - order.height;
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
                text: rail.groups.length + (rail.groups.length === 1 ? " item" : " items")
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
            spacing: 8
            model: rail.groups
            currentIndex: rail.groups.findIndex(group => group.slides.some(slide => slide.index
                                                                                    === rail.controller.liveIndex))

            ScrollBar.vertical: ScrollBar {
            }
            delegate: Column {
                id: groupRow

                required property var modelData
                required property int index
                readonly property bool startsSection: modelData.section.length > 0 && (index === 0
                                                                                       || rail.groups[index
                                                                                                      - 1].section
                                                                                       !== modelData.section)
                readonly property bool opened: !!rail.expanded[modelData.id]
                readonly property bool containsLive: modelData.slides.some(slide => slide.index
                                                                                    === rail.controller.liveIndex)

                objectName: "liveServiceGroup_" + modelData.id
                width: order.width - 6
                spacing: 2

                Text {
                    visible: groupRow.startsSection
                    text: groupRow.modelData.section
                    color: LibraryStyle.muted
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    width: parent.width
                    elide: Text.ElideRight
                    topPadding: 6
                    bottomPadding: 6
                }

                ItemDelegate {
                    id: groupToggle

                    objectName: "liveServiceToggle_" + groupRow.modelData.id
                    width: groupRow.width
                    height: Math.max(44, groupTitle.implicitHeight + 22)
                    padding: 8
                    focusPolicy: Qt.StrongFocus
                    Accessible.role: Accessible.Button
                    Accessible.checkable: true
                    Accessible.checked: groupRow.opened
                    Accessible.name: (groupRow.opened ? "Collapse " : "Expand ")
                                     + groupRow.modelData.title + ", "
                                     + groupRow.modelData.slides.length + " slides" + (
                                         groupRow.modelData.quickAddition ? ", Quick addition" : "")

                    background: Rectangle {
                        radius: 6
                        color: groupRow.containsLive ? LibraryStyle.hover : groupToggle.hovered
                                                       ? LibraryStyle.field : "transparent"
                        border.width: groupToggle.visualFocus ? 1 : 0
                        border.color: LibraryStyle.focus
                    }
                    contentItem: RowLayout {
                        spacing: 6

                        StudioIcon {
                            name: groupRow.opened ? "chevron-down" : "chevron-right"
                        }

                        Text {
                            id: groupTitle

                            text: groupRow.modelData.title
                            font.weight: Font.Medium
                            color: LibraryStyle.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        StudioIcon {
                            visible: groupRow.modelData.quickAddition
                            name: "plus"
                            width: 12
                            height: 12
                            tone: LibraryStyle.muted
                        }

                        Text {
                            text: (groupRow.containsLive ? (groupRow.modelData.slides.findIndex(
                                                                slide => slide.index
                                                                         === rail.controller.liveIndex)
                                                            + 1) + " / " : "")
                                  + groupRow.modelData.slides.length
                            ToolTip.visible: countHover.hovered
                            ToolTip.text: groupRow.modelData.slides.length + " slides" + (
                                              groupRow.modelData.quickAddition
                                              ? " · Quick addition" : "")
                            color: groupRow.containsLive ? rail.currentAccent : LibraryStyle.muted
                            font.pixelSize: 10

                            HoverHandler {
                                id: countHover
                            }
                        }
                    }

                    Accessible.onPressAction: clicked()
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
                        width: groupRow.width - 12
                        x: 12
                        height: Math.max(34, slideCopy.implicitHeight + 12)
                        leftPadding: 12
                        rightPadding: 8
                        topPadding: 6
                        bottomPadding: 6
                        focusPolicy: Qt.StrongFocus
                        Accessible.role: Accessible.Button
                        Accessible.selected: current
                        ToolTip.visible: hovered
                        ToolTip.text: modelData.title
                        ToolTip.delay: 600
                        Accessible.name: "Present slide " + (modelData.index + 1) + ": "
                                         + modelData.title

                        background: Rectangle {
                            radius: 5
                            color: child.current ? rail.currentFill : child.hovered
                                                   ? LibraryStyle.field : "transparent"
                            border.color: child.visualFocus ? LibraryStyle.focus : "transparent"

                            Rectangle {
                                width: 3
                                height: parent.height - 14
                                radius: 1.5
                                x: 0
                                anchors.verticalCenter: parent.verticalCenter
                                color: rail.currentAccent
                                visible: child.current
                            }

                            Rectangle {
                                width: 1
                                height: parent.height + 2
                                x: -7
                                color: LibraryStyle.divider
                            }
                        }
                        contentItem: RowLayout {
                            Text {
                                text: String(child.index + 1).padStart(2, "0")
                                color: child.current ? rail.currentAccent : LibraryStyle.muted
                                font.pixelSize: 10
                                horizontalAlignment: Text.AlignRight
                                Layout.preferredWidth: 18
                            }

                            Text {
                                id: slideCopy

                                text: rail.slideLabel(child.modelData, groupRow.modelData)
                                color: LibraryStyle.text
                                font.pixelSize: 11
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Accessible.onPressAction: rail.chosen(modelData.index)
                        onClicked: rail.chosen(modelData.index)
                    }
                }
            }

            onCurrentIndexChanged: Qt.callLater(rail.scrollToCurrent)

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
