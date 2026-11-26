pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models

Item {
    id: renderer

    property var elements: []
    property var backgroundFill: ({})
    property color backgroundColor: "#54435F"
    property real ambientOpacity: 0.85
    property bool playing: false
    property bool audible: false
    property bool videoEnabled: true
    property bool decodeAudio: true
    property bool showErrors: true
    property bool clearText: false
    property bool transparentBackground: false
    property bool showTransparencyGrid: false
    property bool textOnly: false
    property string suppressedId: ""
    property string gestureId: ""
    property var gestureGeometry: ({})
    property bool ready: false

    signal mediaProblem(string message)

    function probe() {
        let result = [];
        for (let i = 0; i < visualLayers.count; i++) {
            let layer = visualLayers.itemAt(i);
            result.push({
                            uid: layer.layerData.uid,
                            kind: layer.layerData.kind,
                            token: layer.instanceToken,
                            position: layer.playbackPosition,
                            frames: layer.decodedFrames,
                            problem: layer.mediaError
                        });
        }
        return result;
    }

    // Preserve media delegates when unrelated text or geometry changes.
    function reconcile() {
        if (!ready)
            return;
        const incoming = (elements || []).filter(item => !textOnly || item.kind === "text");
        for (let i = 0; i < incoming.length; i++) {
            let item = incoming[i], key = item.uid || String(i), existing = -1;
            for (let j = i; j < layers.count; j++)
                if (layers.get(j).key === key) {
                    existing = j;
                    break;
                }
            if (existing < 0)
                layers.insert(i, {
                                  key: key,
                                  payload: item
                              });
            else {
                if (existing !== i)
                    layers.move(existing, i, 1);
                layers.setProperty(i, "payload", item);
            }
        }
        if (layers.count > incoming.length)
            layers.remove(incoming.length, layers.count - incoming.length);
    }

    width: 1920
    height: 1080

    onElementsChanged: reconcile()
    onTextOnlyChanged: reconcile()
    Component.onCompleted: {
        ready = true;
        reconcile();
    }

    ListModel {
        id: layers

        dynamicRoles: true
    }

    TransparencyGrid {
        anchors.fill: parent
        visible: renderer.showTransparencyGrid && (renderer.backgroundColor.a < 1 || ["linear",
                                                                                      "radial"].includes(
                                                       renderer.backgroundFill.type)) &&
                 !renderer.transparentBackground
    }

    Item {
        anchors.fill: parent
        visible: !renderer.transparentBackground
        opacity: renderer.backgroundColor.a
        // Flatten semi-transparent fills so the ambient pattern shares their alpha.
        layer.enabled: renderer.backgroundColor.a > 0 && renderer.backgroundColor.a < 1

        CanvasFill {
            anchors.fill: parent
            definition: renderer.backgroundFill
            baseColor: renderer.backgroundColor
        }

        Image {
            anchors.fill: parent
            source: "../assets/ambient.svg"
            opacity: renderer.ambientOpacity
            sourceSize.width: 1920
            sourceSize.height: 1080
        }
    }

    Item {
        anchors.fill: parent
        clip: true

        Repeater {
            id: visualLayers

            model: layers

            delegate: SlideLayer {
                required property var payload
                required property int index

                layerData: payload
                visible: layerData.visible !== false && (!renderer.textOnly || layerData.kind
                                                         === "text")
                x: layerData.uid === renderer.gestureId ? renderer.gestureGeometry.x : layerData.x
                y: layerData.uid === renderer.gestureId ? renderer.gestureGeometry.y : layerData.y
                width: layerData.uid === renderer.gestureId ? renderer.gestureGeometry.width :
                                                              layerData.width
                height: layerData.uid === renderer.gestureId ? renderer.gestureGeometry.height :
                                                               layerData.height
                z: index
                playing: renderer.playing
                audible: renderer.audible
                decodeAudio: renderer.decodeAudio
                videoEnabled: renderer.videoEnabled && !renderer.textOnly
                showErrors: renderer.showErrors
                suppressed: layerData.uid === renderer.suppressedId || (renderer.clearText
                                                                        && layerData.kind
                                                                        === "text")

                onMediaProblem: message => renderer.mediaProblem(message)
            }
        }
    }
}
