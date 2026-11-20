pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: scene

    required property var controller
    property bool overlayOnly: false
    property bool blackout: false
    property bool verificationPattern: false

    function probe() {
        return broadcastRenderer.probe();
    }

    width: 1920
    height: 1080

    SlideRenderer {
        id: broadcastRenderer

        elements: scene.controller.liveElements
        backgroundColor: scene.controller.liveBackground
        ambientOpacity: scene.controller.liveAmbientOpacity
        transparentBackground: scene.overlayOnly
        textOnly: scene.overlayOnly
        clearText: scene.controller.textCleared
        playing: scene.controller.playing
        audible: false
        showErrors: false
        visible: !scene.blackout
    }

    Rectangle {
        anchors.fill: parent
        color: scene.overlayOnly ? "transparent" : "#000000"
        visible: scene.blackout
    }

    Rectangle {
        width: 60
        height: 60
        color: "#FF0000"
        visible: scene.verificationPattern
    }

    Rectangle {
        width: 60
        height: 60
        y: scene.height - height
        color: "#0000FF"
        visible: scene.verificationPattern
    }
}
