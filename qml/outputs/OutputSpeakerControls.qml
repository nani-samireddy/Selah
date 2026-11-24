pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: speaker

    required property var controller

    objectName: "outputSpeakerControls"
    spacing: 16

    Text {
        text: "Messages and timers are visible only to the speaker."
        color: LibraryStyle.muted
        font.family: LibraryStyle.fontFamily
        font.pixelSize: 12
    }

    OutputGroup {
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "Speaker notes"
                color: LibraryStyle.text
                font.pixelSize: 15
                font.weight: Font.Medium
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Stage preview"
                iconName: "monitor-play"
                enabled: speaker.controller.liveCount > 0

                onClicked: speaker.controller.preview("stage")
            }
        }

        Text {
            Layout.fillWidth: true
            text: speaker.controller.liveNotes || "No notes for this slide"
            textFormat: Text.PlainText
            color: LibraryStyle.muted
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }

    OutputGroup {
        Text {
            text: "Speaker message"
            color: LibraryStyle.text
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 15
            font.weight: Font.Medium
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryField {
                id: message

                objectName: "outputSpeakerMessage"
                Layout.fillWidth: true
                text: speaker.controller.stageMessage
                placeholderText: "Send a message to the speaker…"
                Accessible.name: "Speaker message"

                onAccepted: speaker.controller.stageMessage = text
            }

            LibraryButton {
                objectName: "outputSendMessage"
                text: "Send"

                onClicked: speaker.controller.stageMessage = message.text
            }

            LibraryButton {
                objectName: "outputClearMessage"
                text: "Clear"

                onClicked: {
                    speaker.controller.stageMessage = "";
                    message.text = "";
                }
            }
        }
    }

    OutputGroup {
        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5

                Text {
                    text: "Countdown"
                    color: LibraryStyle.text
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 15
                    font.weight: Font.Medium
                }

                Text {
                    text: "Set the speaking time in minutes."
                    color: LibraryStyle.muted
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: speaker.controller.countdown
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 32
            }
        }

        RowLayout {
            LibrarySpinBox {
                id: minutes

                objectName: "outputCountdownMinutes"
                from: 1
                to: 180
                value: 20
                Accessible.name: "Countdown minutes"
            }

            Text {
                text: "min"
                color: LibraryStyle.muted
                font.pixelSize: 12
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                objectName: "outputStartCountdown"
                text: "Start"

                onClicked: speaker.controller.startCountdown(minutes.value)
            }

            LibraryButton {
                objectName: "outputPauseCountdown"
                text: "Pause"

                onClicked: speaker.controller.stopCountdown()
            }
        }
    }
}
