pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

SquircleSurface {
    id: panel

    required property var controller
    property string feedback: ""
    property bool wasImporting: false

    color: LibraryStyle.panel
    radius: 10
    border.color: LibraryStyle.divider
    implicitHeight: body.implicitHeight + 24

    FileDialog {
        id: chooseImage

        title: "Choose standby media"
        nameFilters:
            ["Images and videos (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.svg *.mp4 *.mov *.m4v *.webm *.mkv *.avi)"]

        onAccepted: {
            panel.feedback = "";
            panel.controller.importStandbyMedia(selectedFile);
        }
    }

    Connections {
        function onStateChanged() {
            if (panel.wasImporting && !panel.controller.importingStandbyMedia)
                panel.feedback = panel.controller.status;
            panel.wasImporting = panel.controller.importingStandbyMedia;
        }

        target: panel.controller
    }

    ColumnLayout {
        id: body

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 8

        RowLayout {
            StudioIcon {
                name: "image"
            }

            Text {
                text: "Standby media"
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            StudioButton {
                iconName: "trash-2"
                hint: "Remove standby media preference"
                enabled: !panel.controller.importingStandbyMedia
                quiet: true
                visible: panel.controller.defaultImageSource.toString().length > 0

                onClicked: panel.controller.removeDefaultImage()
            }
        }

        Rectangle {
            id: imageFrame

            Layout.fillWidth: true
            Layout.preferredHeight: width * 9 / 16
            color: "#000000"
            radius: 5
            clip: true

            MediaLayer {
                anchors.fill: parent
                sourceUrl: panel.controller.defaultImageSource
                kind: panel.controller.standbyMediaKind
                fit: panel.controller.defaultImageFit
                playing: panel.visible
                audible: false
                looping: true
                videoEnabled: panel.visible && panel.controller.defaultImageAvailable

                onMediaProblem: message => panel.feedback = message
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 24
                visible: !panel.controller.defaultImageAvailable
                text: panel.controller.defaultImageSource.toString().length
                      ? "Saved media is missing" : "Church logo, welcome image or looping video"
                color: "#969696"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }

        Text {
            text: panel.controller.defaultImageName || "Show between service items"
            color: LibraryStyle.muted
            font.pixelSize: 10
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }

        RowLayout {
            StudioButton {
                text: panel.controller.importingStandbyMedia ? "Importing…" :
                                                               panel.controller.defaultImageAvailable
                                                               ? "Change" : "Choose media"
                enabled: !panel.controller.importingStandbyMedia
                iconName: "upload"
                Layout.fillWidth: true

                onClicked: chooseImage.open()
            }

            StudioButton {
                text: "Fit"
                checked: panel.controller.defaultImageFit === "contain"
                hint: "Fit the whole image or video"
                enabled: panel.controller.defaultImageAvailable

                onClicked: panel.controller.defaultImageFit = "contain"
            }

            StudioButton {
                text: "Fill"
                checked: panel.controller.defaultImageFit === "cover"
                hint: "Fill the screen, cropping the edges"
                enabled: panel.controller.defaultImageAvailable

                onClicked: panel.controller.defaultImageFit = "cover"
            }
        }

        StudioButton {
            objectName: "showDefaultImageButton"
            text: panel.controller.defaultImageShowing ? (panel.controller.liveCount
                                                          ? "Resume presentation" :
                                                            "Hide standby media") :
                                                         "Show standby media"
            iconName: panel.controller.defaultImageShowing ? "play" : "image"
            checked: panel.controller.defaultImageShowing
            accented: !panel.controller.defaultImageShowing
            enabled: panel.controller.defaultImageAvailable || panel.controller.defaultImageShowing
            Layout.fillWidth: true

            onClicked: panel.feedback = panel.controller.showDefaultImage(
                           !panel.controller.defaultImageShowing) ? "" : panel.controller.status
        }

        Text {
            visible: panel.feedback.length > 0 || panel.controller.defaultImageShowing
                     || panel.controller.importingStandbyMedia
            text: panel.feedback || (panel.controller.importingStandbyMedia
                                     ? panel.controller.status :
                                       "Changes apply the next time you show standby media. Videos loop silently.")
            color: panel.feedback ? (LibraryStyle.dark ? "#D4B993" : "#7D572A") : LibraryStyle.muted
            font.pixelSize: 10
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
