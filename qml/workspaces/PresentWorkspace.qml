pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: present

    required property var host
    property var controller: host.outputs
    readonly property bool controlHasFocus: {
        let item = host.activeFocusItem;
        return !!item && (item.pressed !== undefined || item.cursorPosition !== undefined
                          || item.currentIndex !== undefined);
    }
    property bool canNavigate: visible && !host.settingsOpen && !controlHasFocus
                               && controller.liveCount > 0

    function navigate(delta) {
        controller.advance(delta);
        forceActiveFocus();
    }

    Shortcut {
        sequence: "Right"
        enabled: present.canNavigate

        onActivated: present.navigate(1)
    }

    Shortcut {
        sequence: "Space"
        enabled: present.canNavigate

        onActivated: present.navigate(1)
    }

    Shortcut {
        sequence: "Left"
        enabled: present.canNavigate

        onActivated: present.navigate(-1)
    }

    Shortcut {
        sequence: "Home"
        enabled: present.canNavigate

        onActivated: present.navigate(-present.controller.liveIndex)
    }

    Shortcut {
        sequence: "End"
        enabled: present.canNavigate

        onActivated: present.navigate(present.controller.liveCount - 1
                                      - present.controller.liveIndex)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 22

        Rectangle {
            Layout.preferredWidth: present.width > 1300 ? 280 : 245
            Layout.fillHeight: true
            color: LibraryStyle.tone("#191919")
            radius: 10
            border.color: LibraryStyle.tone("#323232")

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                Text {
                    text: "Live service"
                    color: LibraryStyle.tone("#EEEEEE")
                    font.pixelSize: 18
                }

                Text {
                    text: "Selecting a slide here changes the live output."
                    color: LibraryStyle.tone("#939393")
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                ListView {
                    id: liveOrder

                    objectName: "liveServiceOrder"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 5
                    model: present.controller.liveOrder
                    currentIndex: present.controller.liveIndex

                    ScrollBar.vertical: ScrollBar {
                    }
                    delegate: ItemDelegate {
                        id: liveRow

                        required property var modelData
                        required property int index

                        width: ListView.view.width - 8
                        height: 64
                        Accessible.name: "Present slide " + (index + 1) + ": " + modelData.title

                        background: Rectangle {
                            radius: 6
                            color: liveRow.index === present.controller.liveIndex ? "#2C3C32" :
                                                                                    liveRow.hovered
                                                                                    ? LibraryStyle.tone(
                                                                                          "#292929") :
                                                                                      LibraryStyle.tone(
                                                                                          "#202020")
                            border.color: liveRow.index === present.controller.liveIndex
                                          ? "#658772" : LibraryStyle.tone("#303030")
                        }
                        contentItem: Column {
                            spacing: 6

                            Text {
                                width: parent.width
                                text: String(liveRow.index + 1).padStart(2, "0") + "  "
                                      + liveRow.modelData.title
                                color: LibraryStyle.tone("#E0E0E0")
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: liveRow.modelData.itemTitle || "Presentation"
                                color: "#969F98"
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }

                        onClicked: present.navigate(index - present.controller.liveIndex)
                    }

                    onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
                }

                StudioButton {
                    text: "Prepare a program"
                    Layout.fillWidth: true

                    onClicked: present.host.showPrograms()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            RowLayout {
                Layout.fillWidth: true

                Column {
                    spacing: 5

                    Text {
                        text: "Present"
                        color: LibraryStyle.tone("#EEEEEE")
                        font.pixelSize: 24
                    }

                    Text {
                        text: present.controller.liveCount ? "Slide " + (
                                                                 present.controller.liveIndex + 1)
                                                             + " of "
                                                             + present.controller.liveCount
                                                             + " · Audience " + (
                                                                 present.controller.audienceEnabled
                                                                 ? "on" : "stopped") + " · Stage "
                                                             + (present.controller.stageEnabled
                                                                ? "on" : "stopped") :
                                                             "Prepare a program, then choose Present from here."
                        color: LibraryStyle.tone("#999999")
                        font.pixelSize: 12
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                StudioButton {
                    text: "Audience preview"
                    enabled: present.controller.liveCount > 0

                    onClicked: present.controller.preview("audience")
                }

                StudioButton {
                    text: "Stage preview"
                    enabled: present.controller.liveCount > 0

                    onClicked: present.controller.preview("stage")
                }
            }

            ScrollView {
                id: previewScroll

                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: previewScroll.availableWidth
                    spacing: 20

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 18

                        LiveSlideCard {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            Layout.preferredWidth: 3
                            controller: present.controller
                        }

                        LiveSlideCard {
                            Layout.alignment: Qt.AlignTop
                            Layout.fillWidth: true
                            Layout.preferredWidth: 2
                            controller: present.controller
                            next: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: stageContent.implicitHeight + 32
                        color: LibraryStyle.tone("#191919")
                        radius: 8
                        border.color: LibraryStyle.tone("#333333")

                        RowLayout {
                            id: stageContent

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 16
                            spacing: 24

                            ColumnLayout {
                                Layout.alignment: Qt.AlignTop
                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    text: "SPEAKER NOTES"
                                    color: LibraryStyle.tone("#969696")
                                    font.pixelSize: 10
                                    font.letterSpacing: 1
                                }

                                Text {
                                    text: present.controller.liveNotes || "No notes for this slide"
                                    textFormat: Text.PlainText
                                    color: LibraryStyle.tone("#CDCDCD")
                                    font.pixelSize: 13
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }

                                Text {
                                    visible: present.controller.stageMessage.length > 0
                                    text: "Stage message: " + present.controller.stageMessage
                                    textFormat: Text.PlainText
                                    color: "#D4B993"
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }

                            Column {
                                Layout.alignment: Qt.AlignTop
                                spacing: 8

                                Text {
                                    text: "COUNTDOWN"
                                    color: LibraryStyle.tone("#969696")
                                    font.pixelSize: 10
                                    font.letterSpacing: 1
                                }

                                Text {
                                    text: present.controller.countdown
                                    color: LibraryStyle.tone("#EEEEEE")
                                    font.pixelSize: 32
                                }

                                StudioButton {
                                    text: "Stage controls"

                                    onClicked: present.host.showOutputs()
                                }
                            }
                        }
                    }

                    Text {
                        text: "Still previews · Video and audio play on the presentation outputs"
                        color: LibraryStyle.tone("#818181")
                        font.pixelSize: 11
                    }
                }
            }

            Text {
                visible: present.controller.audienceBlack || present.controller.textCleared
                text: present.controller.audienceBlack
                      ? "Audience is blacked out. Restore it when you are ready." :
                        "Audience text is cleared. Background media continues."
                color: "#E1B78B"
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                StudioButton {
                    text: "Previous"
                    implicitHeight: 44
                    enabled: present.controller.liveIndex > 0

                    onClicked: present.navigate(-1)
                }

                StudioButton {
                    text: "Next slide"
                    accented: true
                    implicitHeight: 44
                    Layout.preferredWidth: 160
                    enabled: present.controller.liveIndex >= 0 && present.controller.liveIndex
                             < present.controller.liveCount - 1

                    onClicked: present.navigate(1)
                }

                Item {
                    Layout.fillWidth: true
                }

                StudioButton {
                    text: present.controller.playing ? "Pause media" : "Play media"
                    enabled: present.controller.liveCount > 0

                    onClicked: present.controller.playing = !present.controller.playing
                }

                StudioButton {
                    text: present.controller.textCleared ? "Restore text" : "Clear text"
                    checked: present.controller.textCleared
                    enabled: present.controller.liveCount > 0

                    onClicked: present.controller.clearText(!present.controller.textCleared)
                }

                StudioButton {
                    text: present.controller.audienceBlack ? "Restore audience" : "Blackout"
                    checked: present.controller.audienceBlack
                    implicitHeight: 44
                    enabled: present.controller.liveCount > 0

                    onClicked: present.controller.blackout("audience",
                                                           !present.controller.audienceBlack)
                }
            }

            Text {
                text: "← Previous    → / Space Next    Home / End First / last"
                color: LibraryStyle.tone("#838383")
                font.pixelSize: 11
            }
        }
    }
}
