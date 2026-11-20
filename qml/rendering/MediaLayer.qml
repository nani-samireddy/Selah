pragma ComponentBehavior: Bound
import QtQuick
import QtMultimedia

Item {
    id: media

    property url sourceUrl
    property string kind: "image"
    property string fit: "contain"
    property bool playing: false
    property bool audible: false
    property bool looping: true
    property bool videoEnabled: true
    property bool showErrors: true
    property string problem: ""
    readonly property var loadedVideo: videoLoader.item
    readonly property real playbackPosition: loadedVideo ? loadedVideo.playbackPosition : 0
    readonly property int decodedFrames: loadedVideo ? loadedVideo.decodedFrames : 0

    signal mediaProblem(string message)

    clip: true

    onSourceUrlChanged: problem = ""

    Image {
        anchors.fill: parent
        visible: media.kind === "image"
        source: visible ? media.sourceUrl : ""
        asynchronous: true
        sourceSize.width: 3840
        sourceSize.height: 2160
        autoTransform: true
        fillMode: media.fit === "cover" ? Image.PreserveAspectCrop : media.fit === "stretch"
                                          ? Image.Stretch : Image.PreserveAspectFit

        onStatusChanged: if (status === Image.Error) {
                             media.problem = "Image unavailable";
                             media.mediaProblem(media.problem);
                         }
    }

    Loader {
        id: videoLoader

        anchors.fill: parent
        active: media.kind === "video" && media.videoEnabled

        sourceComponent: Component {
            Item {
                id: videoContainer

                readonly property real playbackPosition: player.position
                property int decodedFrames: 0

                VideoOutput {
                    id: video

                    anchors.fill: parent
                    fillMode: media.fit === "cover" ? VideoOutput.PreserveAspectCrop : media.fit
                                                      === "stretch" ? VideoOutput.Stretch :
                                                                      VideoOutput.PreserveAspectFit
                }

                MediaPlayer {
                    id: player

                    source: media.sourceUrl
                    videoOutput: video
                    loops: media.looping ? MediaPlayer.Infinite : 1

                    audioOutput: AudioOutput {
                        muted: !media.audible
                        volume: 1
                    }

                    onMediaStatusChanged: if (mediaStatus === MediaPlayer.LoadedMedia)
                                              play()
                    onErrorOccurred: {
                        media.problem = "Video unavailable: " + errorString;
                        media.mediaProblem(media.problem);
                    }
                }

                Connections {
                    function onPlayingChanged() {
                        if (media.playing)
                            player.play();
                        else
                            player.pause();
                    }

                    target: media
                }

                Connections {
                    function onVideoFrameChanged() {
                        videoContainer.decodedFrames++;
                        if (!media.playing && player.playbackState === MediaPlayer.PlayingState)
                            player.pause();
                    }

                    target: video.videoSink
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: (media.problem.length > 0 && media.showErrors) || (media.kind === "video" &&
                                                                    !media.videoEnabled)
        color: "#191919"

        Text {
            anchors.centerIn: parent
            width: parent.width - 40
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: media.problem.length ? media.problem : "VIDEO"
            color: "#AAAAAA"
            font.pixelSize: Math.max(20, Math.min(38, media.height / 8))
        }
    }
}
