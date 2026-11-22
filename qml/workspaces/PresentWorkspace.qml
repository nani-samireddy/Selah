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

    objectName: "presentWorkspace"

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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true

                Text {
                    text: "Present"
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Medium
                    color: LibraryStyle.text
                }

                Text {
                    text: present.controller.liveCount ? present.controller.liveProgramTitle
                                                         + " · " + present.controller.liveCount
                                                         + " slides" :
                                                         "Choose a program or show your default image."
                    font.pixelSize: 11
                    color: LibraryStyle.muted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Text {
                text: present.controller.audienceEnabled ? "● Audience on" : "○ Audience off"
                color: present.controller.audienceEnabled ? (LibraryStyle.dark ? "#A8CFB6" :
                                                                                 "#306044") :
                                                            LibraryStyle.muted
                font.pixelSize: 11
            }

            StudioButton {
                text: "Audience preview"
                iconName: "monitor-play"
                enabled: present.controller.hasAudienceContent

                onClicked: present.controller.preview("audience")
            }
        }

        RowLayout {
            id: body

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: 0
            spacing: 16

            PresentSlideList {
                controller: present.controller
                Layout.preferredWidth: present.width > 1250 ? 232 : 205
                Layout.fillHeight: true

                onChosen: index => present.navigate(index - present.controller.liveIndex)
                onPrepareRequested: present.host.showPrograms()
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                spacing: 16

                LiveSlideCard {
                    controller: present.controller
                    previewHeightLimit: Math.max(150, body.height - 190)
                    Layout.fillWidth: true
                }

                Item {
                    Layout.fillHeight: true
                }

                PresentNotes {
                    controller: present.controller
                    Layout.fillWidth: true

                    onControlsRequested: present.host.showOutputs()
                }
            }

            ScrollView {
                id: sideScroll

                Layout.preferredWidth: present.width > 1250 ? 270 : 235
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: sideScroll.availableWidth
                    spacing: 12

                    LiveSlideCard {
                        controller: present.controller
                        next: true
                        previewHeightLimit: present.width > 1250 ? 10000 : 118
                        Layout.fillWidth: true
                    }

                    PresentImagePanel {
                        controller: present.controller
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: present.controller.audienceBlack || present.controller.textCleared
                     || present.controller.defaultImageShowing
            text: present.controller.audienceBlack
                  ? "Audience is blacked out. Restore it when you’re ready." :
                    present.controller.defaultImageShowing
                    ? "Default image is showing · Resume returns to the same program slide." :
                      "Audience text is cleared. Background media continues."
            color: LibraryStyle.dark ? "#D4B993" : "#7D572A"
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }

        PresentTransport {
            controller: present.controller
            Layout.fillWidth: true

            onNavigate: delta => present.navigate(delta)
            onOutputsRequested: present.host.showOutputs()
        }
    }
}
