pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: scene

    required property var controller
    property bool overlayOnly: false
    property bool blackout: false
    readonly property bool effectiveBlackout: blackout || controller.audienceBlack
    property bool verificationPattern: false

    function probe() {
        return broadcastRenderer.probe();
    }

    width: 1920
    height: 1080

    AudienceRenderer {
        id: broadcastRenderer

        controller: scene.controller
        transparentBackground: scene.overlayOnly
        audible: false
        visible: !scene.effectiveBlackout
    }

    Rectangle {
        anchors.fill: parent
        color: scene.overlayOnly ? "transparent" : "#000000"
        visible: scene.effectiveBlackout
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
