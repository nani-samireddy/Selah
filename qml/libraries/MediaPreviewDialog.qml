pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

// Inspect originals without changing the editor, program, or live snapshot.
Popup {
    id: viewer

    property var asset: ({})
    property bool nativeSize: false
    readonly property bool audio: asset.kind === "audio"
    readonly property bool playable: video || audio
    readonly property bool video: asset.kind === "video"

    objectName: "mediaPreviewDialog"
    parent: Overlay.overlay
    width: Math.min(viewer.audio ? 720 : 1500, parent.width - 40)
    height: viewer.audio ? Math.min(420, parent.height - 40) : parent.height - 40
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    padding: 16
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: SquircleSurface {
        radius: 10
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
    }

    onOpened: {
        nativeSize = false;
        sound.muted = true;
    }
    onClosed: {
        player.stop();
        asset = ({});
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true

                Text {
                    text: viewer.asset.name || "Media preview"
                    color: LibraryStyle.text
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 18
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Text {
                    text: viewer.audio ? "Original audio · press Play, then Unmute to listen" :
                                         viewer.video ? "Original video" : original.status
                                                        === Image.Ready ? original.sourceSize.width
                                                                          + " × " + original.sourceSize.height
                                                                          + " · Original image" :
                                                                          "Loading original…"
                    color: LibraryStyle.muted
                    font.pixelSize: 11
                }
            }

            LibraryButton {
                visible: !viewer.playable
                objectName: "mediaPreviewZoom"
                text: viewer.nativeSize ? "Fit to window" : "100%"
                enabled: original.status === Image.Ready

                onClicked: viewer.nativeSize = !viewer.nativeSize
            }

            LibraryButton {
                text: "Close"

                onClicked: viewer.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#111111"
            radius: 6
            clip: true

            Flickable {
                id: imageViewport

                anchors.fill: parent
                visible: !viewer.playable
                contentWidth: Math.max(width, original.width)
                contentHeight: Math.max(height, original.height)
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                }
                ScrollBar.horizontal: ScrollBar {
                }

                onWidthChanged: returnToBounds()
                onHeightChanged: returnToBounds()

                Image {
                    id: original

                    objectName: "mediaOriginalImage"
                    source: viewer.opened && !viewer.playable ? viewer.asset.source || "" : ""
                    asynchronous: true
                    // No sourceSize cap: the 100% view uses the original file's pixels.
                    width: viewer.nativeSize ? implicitWidth : imageViewport.width
                    height: viewer.nativeSize ? implicitHeight : imageViewport.height
                    x: Math.max(0, (imageViewport.width - width) / 2)
                    y: Math.max(0, (imageViewport.height - height) / 2)
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
            }

            VideoOutput {
                id: previewVideo

                objectName: "mediaPreviewVideo"
                anchors.fill: parent
                visible: viewer.video
                fillMode: VideoOutput.PreserveAspectFit
            }

            Column {
                visible: viewer.audio
                anchors.centerIn: parent
                spacing: 16

                StudioIcon {
                    name: "music"
                    width: 64
                    height: 64
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: viewer.asset.name || "Audio"
                    color: LibraryStyle.text
                    font.pixelSize: 20
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: viewer.opened && !viewer.playable && original.status === Image.Loading
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 48
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: viewer.playable ? player.errorString :
                                        "Unable to open this image. Check that the original file is available."
                visible: viewer.playable ? player.error !== MediaPlayer.NoError : original.status
                                           === Image.Error
                color: "#EAA5A5"
                font.pixelSize: 12
            }
        }

        RowLayout {
            visible: viewer.playable
            Layout.fillWidth: true

            LibraryButton {
                objectName: "mediaPreviewPlay"
                text: player.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"
                iconName: player.playbackState === MediaPlayer.PlayingState ? "pause" : "play"

                onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() :
                                                                               player.play()
            }

            Slider {
                Layout.fillWidth: true
                from: 0
                to: player.duration
                value: player.position
                enabled: player.seekable

                onMoved: player.setPosition(value)
            }

            Text {
                text: Math.floor(player.position / 60000) + ":" + Math.floor(player.position / 1000
                                                                             % 60).toString(
                          ).padStart(2, "0") + " / " + Math.floor(player.duration / 60000) + ":"
                      + Math.floor(player.duration / 1000 % 60).toString().padStart(2, "0")
                color: LibraryStyle.muted
                font.pixelSize: 12
            }

            LibraryButton {
                text: sound.muted ? "Unmute" : "Mute"

                onClicked: sound.muted = !sound.muted
            }
        }
    }

    MediaPlayer {
        id: player

        objectName: "mediaPreviewPlayer"
        source: viewer.opened && viewer.playable ? viewer.asset.source || "" : ""
        videoOutput: previewVideo

        audioOutput: AudioOutput {
            id: sound

            muted: true
        }
    }
}
