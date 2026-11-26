pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: stage

    property var controller
    property string clock: Qt.formatTime(new Date(), "hh:mm:ss")

    width: 1920
    height: 1080

    Timer {
        interval: 1000
        running: stage.visible
        repeat: true

        onTriggered: stage.clock = Qt.formatTime(new Date(), "hh:mm:ss")
    }

    Rectangle {
        anchors.fill: parent
        color: "#0C0D10"
    }

    Text {
        x: 70
        y: 45
        text: "STAGE  /  " + (stage.controller.defaultImageShowing ? "STANDBY MEDIA  ·  " : "") + (
                  stage.controller.liveTitle || "Ready")
        width: 1400
        elide: Text.ElideRight
        color: "#929BAA"
        font.pixelSize: 26
        font.letterSpacing: 2
    }

    Text {
        text: stage.clock
        color: "#F0F1F4"
        font.pixelSize: 46

        anchors {
            right: parent.right
            top: parent.top
            rightMargin: 70
            topMargin: 40
        }
    }

    Rectangle {
        x: 70
        y: 125
        width: 1780
        height: 1
        color: "#303640"
    }

    Text {
        x: 70
        y: 170
        text: "CURRENT" + (stage.controller.musicianView ? " / " + stage.controller.stageSection + (
                                                               stage.controller.songKey ? " · Key "
                                                                                          + stage.controller.songKey :
                                                                                          "") : "")
        color: "#A5ADBA"
        font.pixelSize: 24
        font.letterSpacing: 3
    }

    Text {
        x: 70
        y: 225
        width: 1070
        height: 530
        visible: !stage.controller.musicianView
        text: stage.controller.liveText || "Waiting for live content"
        color: "#F3F3F5"
        font.pixelSize: 100
        fontSizeMode: Text.Fit
        minimumPixelSize: 42
        wrapMode: Text.Wrap
        clip: true
    }

    Text {
        x: 1240
        y: 170
        text: "NEXT  /  " + (stage.controller.nextTitle || "End")
        color: "#A5ADBA"
        font.pixelSize: 24
        font.letterSpacing: 2
    }

    Text {
        x: 1240
        y: 225
        width: 600
        height: 250
        visible: !stage.controller.musicianView
        text: stage.controller.nextText || "—"
        color: "#BCC4CF"
        font.pixelSize: 54
        fontSizeMode: Text.Fit
        minimumPixelSize: 28
        wrapMode: Text.Wrap
        clip: true
    }

    ChordLyricsItem {
        x: 70
        y: 225
        width: 1070
        height: 530
        visible: stage.controller.musicianView
        content: stage.controller.stageCurrent
        pixelSize: 84
    }

    ChordLyricsItem {
        x: 1240
        y: 225
        width: 600
        height: 250
        visible: stage.controller.musicianView
        content: stage.controller.stageNext
        pixelSize: 48
        color: "#BCC4CF"
    }

    Text {
        x: 1240
        y: 540
        text: "COUNTDOWN"
        color: "#929BAA"
        font.pixelSize: 24
        font.letterSpacing: 2
    }

    Text {
        x: 1240
        y: 585
        text: stage.controller.countdown
        color: "#E8C987"
        font.pixelSize: 110
        font.family: "Arial"
    }

    Rectangle {
        x: 70
        y: 790
        width: 1780
        height: 1
        color: "#303640"
    }

    Text {
        x: 70
        y: 825
        text: stage.controller.musicianView ? "MUSICIAN CUES" : "SPEAKER NOTES"
        color: "#929BAA"
        font.pixelSize: 23
        font.letterSpacing: 2
    }

    Text {
        x: 70
        y: 870
        width: 1720
        height: 90
        text: (stage.controller.musicianView ? stage.controller.stageCue : "")
              || stage.controller.liveNotes || "No notes for this slide"
        color: "#C3CAD4"
        font.pixelSize: 35
        wrapMode: Text.Wrap
        elide: Text.ElideRight
    }

    Rectangle {
        x: 0
        y: 985
        width: parent.width
        height: 95
        color: stage.controller.stageMessage.length ? "#383128" : "#14171D"

        Text {
            text: stage.controller.stageMessage || ""
            color: "#F7E9CD"
            font.pixelSize: 38
            fontSizeMode: Text.Fit
            minimumPixelSize: 20
            verticalAlignment: Text.AlignVCenter

            anchors {
                fill: parent
                margins: 25
            }
        }
    }
}
