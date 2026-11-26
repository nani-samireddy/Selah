pragma ComponentBehavior: Bound
import QtQuick

// One live renderer/decoder set, with alternating frozen GPU textures. Fades use
// the shared output clock, never a per-window animation or CPU frame readback.
Item {
    id: audience

    required property var controller
    property bool active: true
    property bool playing: controller.defaultImageShowing || controller.playing
    property bool audible: false
    property bool videoEnabled: true
    property bool transparentBackground: false
    property bool frontIsA: true
    property bool participating: false
    readonly property bool shadersAvailable: GraphicsInfo.api !== GraphicsInfo.Software

    function probe() {
        return content.probe();
    }

    width: 1920
    height: 1080

    onActiveChanged: participating = false

    Connections {
        function onTransitionStarting() {
            if (audience.active && audience.shadersAvailable) {
                audience.frontIsA = !audience.frontIsA;
                audience.participating = true;
            }
        }

        function onTransitionChanged() {
            if (!audience.controller.transitioning)
                audience.participating = false;
        }

        target: audience.controller
    }

    SlideRenderer {
        id: content

        elements: audience.controller.audienceElements
        backgroundColor: audience.controller.audienceBackground
        backgroundFill: audience.controller.audienceFill
        ambientOpacity: audience.controller.audienceAmbientOpacity
        clearText: audience.controller.textCleared
        transparentBackground: audience.transparentBackground
        playing: audience.active && audience.playing
        audible: audience.active && audience.audible
        videoEnabled: audience.videoEnabled
        decodeAudio: false
        showErrors: false
    }

    ShaderEffectSource {
        id: bufferA

        anchors.fill: parent
        sourceItem: audience.shadersAvailable ? content : null
        live: audience.frontIsA
        hideSource: audience.shadersAvailable
        visible: false
        textureSize: Qt.size(1920, 1080)
    }

    ShaderEffectSource {
        id: bufferB

        anchors.fill: parent
        sourceItem: audience.shadersAvailable ? content : null
        live: !audience.frontIsA
        hideSource: audience.shadersAvailable
        visible: false
        textureSize: Qt.size(1920, 1080)
    }

    ShaderEffect {
        property var outgoing: audience.frontIsA ? bufferB : bufferA
        property var incoming: audience.frontIsA ? bufferA : bufferB
        property int effect: audience.controller.transitionEffect
        property real progress: audience.participating ? audience.controller.transitionProgress : 1

        anchors.fill: parent
        visible: audience.shadersAvailable
        fragmentShader: "qrc:/shaders/crossfade.frag.qsb"
    }
}
