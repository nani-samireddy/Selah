pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    property var controller
    property var document

    signal remoteRequested
    signal broadcastRequested

    width: Math.min(820, parent.width - 60)
    height: Math.min(720, parent.height - 40)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 20
    modal: true
    focus: true

    background: Rectangle {
        color: "#191919"
        radius: 12
        border.color: "#3C3C3C"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "Outputs & stage"
                font.pixelSize: 20
                color: "#EEEEEE"
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Remote"

                onClicked: panel.remoteRequested()
            }

            StudioButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: panel.availableWidth - 10
                spacing: 18

                RowLayout {
                    Layout.fillWidth: true

                    Column {
                        spacing: 5

                        Text {
                            text: panel.controller.liveTitle.length ? "LIVE  /  "
                                                                      + panel.controller.liveTitle :
                                                                      "No live content"
                            color: "#DDDDDD"
                            font.pixelSize: 13
                        }

                        Text {
                            text: "Draft changes stay off the outputs until you take them live."
                            color: "#8C8C8C"
                            font.pixelSize: 11
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    StudioButton {
                        text: "Take editor slide live"
                        accented: true

                        onClicked: panel.controller.takeLive(panel.document.slides,
                                                             panel.document.currentSlide)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    StudioButton {
                        text: "Previous"
                        enabled: panel.controller.liveIndex > 0

                        onClicked: panel.controller.advance(-1)
                    }

                    StudioButton {
                        text: "Next"
                        enabled: panel.controller.liveIndex >= 0 && panel.controller.liveIndex
                                 < panel.controller.liveCount - 1

                        onClicked: panel.controller.advance(1)
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    StudioButton {
                        text: panel.controller.playing ? "Pause media" : "Play media"

                        onClicked: panel.controller.playing = !panel.controller.playing
                    }

                    StudioButton {
                        text: panel.controller.audioEnabled ? "Audio on" : "Audio off"
                        checked: panel.controller.audioEnabled

                        onClicked: panel.controller.audioEnabled = !panel.controller.audioEnabled
                    }

                    StudioButton {
                        text: panel.controller.textCleared ? "Restore text" : "Clear text"
                        checked: panel.controller.textCleared

                        onClicked: panel.controller.clearText(!panel.controller.textCleared)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: "#363636"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Repeater {
                        model: ["audience", "stage"]

                        delegate: ColumnLayout {
                            id: card

                            required property string modelData
                            property string chosenId: modelData === "audience"
                                                      ? panel.controller.audienceDisplay :
                                                        panel.controller.stageDisplay
                            property bool enabledOutput: modelData === "audience"
                                                         ? panel.controller.audienceEnabled :
                                                           panel.controller.stageEnabled
                            property bool black: modelData === "audience"
                                                 ? panel.controller.audienceBlack :
                                                   panel.controller.stageBlack

                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            spacing: 10

                            Text {
                                text: card.modelData === "audience" ? "Audience" : "Stage / speaker"
                                color: "#DDDDDD"
                                font.pixelSize: 15
                            }

                            Text {
                                text: card.modelData === "audience" ? "Presentation + media" :
                                                                      "Current, next, notes, timers"
                                color: "#888888"
                                font.pixelSize: 11
                            }

                            ComboBox {
                                id: displayChoice

                                Layout.fillWidth: true
                                implicitHeight: 35
                                model: panel.controller.displays
                                textRole: "name"
                                valueRole: "id"
                                currentIndex: {
                                    let selected = panel.controller.displays.findIndex(d => d.id
                                                                                            === card.chosenId);
                                    if (selected >= 0)
                                        return selected;
                                    let external = panel.controller.displays.findIndex(d =>
                                    !d.operator);
                                    return external >= 0 ? external : 0;
                                }

                                background: Rectangle {
                                    color: "#272727"
                                    radius: 5
                                    border.color: displayChoice.visualFocus ? "#888888" : "#414141"
                                }
                                indicator: StudioIcon {
                                    name: "chevron-down"
                                    x: displayChoice.width - 23
                                    y: (displayChoice.height - height) / 2
                                }
                                contentItem: Text {
                                    text: displayChoice.displayText
                                    elide: Text.ElideMiddle
                                    color: "#CCCCCC"
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 9
                                    rightPadding: 26
                                }

                                onActivated: card.chosenId = currentValue
                            }

                            Text {
                                text: displayChoice.currentIndex >= 0
                                      && panel.controller.displays[displayChoice.currentIndex]
                                      ?.operator ? "Operator display · use windowed preview" :
                                                   "External display"
                                color: "#888888"
                                font.pixelSize: 10
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                StudioButton {
                                    text: "Fullscreen"
                                    Layout.fillWidth: true

                                    onClicked: panel.controller.route(card.modelData,
                                                                      displayChoice.currentValue)
                                }

                                StudioButton {
                                    text: "Preview"
                                    Layout.fillWidth: true

                                    onClicked: panel.controller.preview(card.modelData)
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                StudioButton {
                                    text: card.black ? "Restore" : "Blackout"
                                    checked: card.black
                                    Layout.fillWidth: true

                                    onClicked: panel.controller.blackout(card.modelData,
                                                                         !card.black)
                                }

                                StudioButton {
                                    text: "Stop"
                                    enabled: card.enabledOutput
                                    Layout.fillWidth: true

                                    onClicked: panel.controller.stop(card.modelData)
                                }
                            }

                            Text {
                                text: card.enabledOutput ? "Output active" : "Output stopped"
                                color: "#A0A0A0"
                                font.pixelSize: 10
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: "#363636"
                }

                Text {
                    text: "STAGE MESSAGE"
                    color: "#939393"
                    font.pixelSize: 10
                    font.letterSpacing: 1
                }

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: message

                        Layout.fillWidth: true
                        placeholderText: "Message visible only to the speaker…"
                        text: panel.controller.stageMessage
                        color: "#DDDDDD"
                        font.pixelSize: 12
                        selectByMouse: true
                        padding: 10

                        background: Rectangle {
                            color: "#252525"
                            radius: 5
                            border.color: "#3B3B3B"
                        }
                    }

                    StudioButton {
                        text: "Send"

                        onClicked: panel.controller.stageMessage = message.text
                    }

                    StudioButton {
                        text: "Clear"

                        onClicked: {
                            panel.controller.stageMessage = "";
                            message.text = "";
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "Countdown"
                        color: "#A0A0A0"
                        font.pixelSize: 12
                    }

                    SpinBox {
                        id: minutes

                        from: 1
                        to: 180
                        value: 20
                        editable: true
                        implicitWidth: 100
                    }

                    Text {
                        text: "minutes"
                        color: "#888888"
                        font.pixelSize: 11
                    }

                    StudioButton {
                        text: "Start"

                        onClicked: panel.controller.startCountdown(minutes.value)
                    }

                    StudioButton {
                        text: "Pause"

                        onClicked: panel.controller.stopCountdown()
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        text: panel.controller.countdown
                        color: "#DDDDDD"
                        font.pixelSize: 23
                    }
                }

                Text {
                    text: panel.controller.status
                    color: "#B9B9B9"
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: "#363636"
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "NDI network output"
                        color: "#DDDDDD"
                        font.pixelSize: 13
                        Layout.fillWidth: true
                    }

                    StudioButton {
                        text: "Configure prototype"

                        onClicked: {
                            panel.close();
                            panel.broadcastRequested();
                        }
                    }
                }
            }
        }
    }
}
