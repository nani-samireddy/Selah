pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: transport

    required property var controller
    required property bool presenting

    signal navigate(int delta)
    signal outputsRequested
    signal stopRequested

    objectName: "presentTransport"
    implicitHeight: 56
    color: LibraryStyle.panel
    radius: 10
    border.color: LibraryStyle.divider

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 7

        StudioButton {
            objectName: "outputPreviousButton"
            text: "Previous"
            iconName: "chevron-left"
            enabled: transport.controller.liveIndex > 0

            onClicked: transport.navigate(-1)
        }

        StudioButton {
            objectName: "outputNextButton"
            text: "Next slide"
            iconName: "chevron-right"
            accented: true
            Layout.preferredWidth: 130
            enabled: transport.controller.liveIndex >= 0 && transport.controller.liveIndex
                     < transport.controller.liveCount - 1

            onClicked: transport.navigate(1)
        }

        Text {
            text: "←  →  / Space"
            visible: transport.width > 850
            color: LibraryStyle.muted
            font.pixelSize: 10
            Layout.leftMargin: 5
        }

        Item {
            Layout.fillWidth: true
        }

        StudioButton {
            objectName: "outputPlayButton"
            iconName: transport.controller.playing ? "pause" : "play"
            hint: transport.controller.playing ? "Pause media" : "Play media"
            enabled: transport.controller.liveCount > 0 && !transport.controller.defaultImageShowing

            onClicked: transport.controller.playing = !transport.controller.playing
        }

        StudioButton {
            objectName: "outputAudioButton"
            text: transport.controller.audioEnabled ? "Audio on" : "Audio off"
            checked: transport.controller.audioEnabled
            hint: transport.controller.audioEnabled ? "Mute presentation audio" :
                                                      "Enable presentation audio"

            onClicked: transport.controller.audioEnabled = !transport.controller.audioEnabled
        }

        StudioButton {
            objectName: "outputClearTextButton"
            text: transport.controller.textCleared ? "Restore text" : "Clear text"
            iconName: "type"
            checked: transport.controller.textCleared
            enabled: transport.controller.liveCount > 0 && !transport.controller.defaultImageShowing

            onClicked: transport.controller.clearText(!transport.controller.textCleared)
        }

        StudioButton {
            objectName: "outputAudienceBlackout"
            text: transport.controller.audienceBlack ? "Restore" : "Blackout"
            iconName: transport.controller.audienceBlack ? "eye" : "eye-off"
            checked: transport.controller.audienceBlack
            enabled: transport.controller.hasAudienceContent

            onClicked: transport.controller.blackout("audience",
                                                     !transport.controller.audienceBlack)
        }

        StudioButton {
            iconName: "settings-2"
            hint: "Output settings"

            onClicked: transport.outputsRequested()
        }

        StudioButton {
            objectName: "stopPresentingButton"
            text: "Stop presenting"
            iconName: "square"
            destructive: true
            enabled: transport.presenting
            Layout.leftMargin: 8
            hint: "Stop audience, stage and NDI outputs"

            onClicked: transport.stopRequested()
        }
    }
}
