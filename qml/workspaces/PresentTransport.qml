pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: transport

    required property var controller

    signal navigate(int delta)
    signal outputsRequested

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
            text: "Previous"
            iconName: "chevron-left"
            enabled: transport.controller.liveIndex > 0

            onClicked: transport.navigate(-1)
        }

        StudioButton {
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
            iconName: transport.controller.playing ? "pause" : "play"
            hint: transport.controller.playing ? "Pause media" : "Play media"
            enabled: transport.controller.liveCount > 0 && !transport.controller.defaultImageShowing

            onClicked: transport.controller.playing = !transport.controller.playing
        }

        StudioButton {
            text: "Clear text"
            iconName: "type"
            checked: transport.controller.textCleared
            enabled: transport.controller.liveCount > 0 && !transport.controller.defaultImageShowing

            onClicked: transport.controller.clearText(!transport.controller.textCleared)
        }

        StudioButton {
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
    }
}
