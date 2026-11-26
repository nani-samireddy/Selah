pragma ComponentBehavior: Bound
import QtQuick
import ChurchDeck.Formatting

Item {
    id: visualLayer

    property var layerData: ({})
    property bool playing: false
    property bool audible: false
    property bool videoEnabled: true
    property bool decodeAudio: true
    property bool showErrors: true
    property bool suppressed: false
    readonly property string instanceToken: Math.random().toString(36).slice(2)
    readonly property var loadedContent: content.item
    readonly property real playbackPosition: loadedContent && (layerData.kind === "video"
                                                               || layerData.kind === "audio")
                                             ? loadedContent.playbackPosition : 0
    readonly property int decodedFrames: loadedContent && (layerData.kind === "video"
                                                           || layerData.kind === "audio")
                                         ? loadedContent.decodedFrames : 0
    readonly property string mediaError: loadedContent && layerData.kind !== "text"
                                         ? loadedContent.problem : ""

    signal mediaProblem(string message)

    x: layerData.x || 0
    y: layerData.y || 0
    width: layerData.width || 100
    height: layerData.height || 100
    rotation: layerData.rotation || 0
    visible: layerData.visible !== false
    opacity: suppressed ? 0 : (layerData.opacity === undefined ? 1 : layerData.opacity)

    Loader {
        id: content

        anchors.fill: parent
        sourceComponent: visualLayer.layerData.kind === "image" || visualLayer.layerData.kind
                         === "video" || visualLayer.layerData.kind === "audio" ? media : text
    }

    Component {
        id: text

        TextEdit {
            id: renderedText

            readonly property var displayData: TextFormatting.display(visualLayer.layerData)

            function restoreFormatting() {
                TextFormatting.hydrate(textDocument, displayData);
            }

            objectName: "slide-text-" + visualLayer.layerData.uid
            textFormat: TextEdit.RichText
            text: TextFormatting.html(displayData)
            color: visualLayer.layerData.color || "#FFFFFF"
            font.family: visualLayer.layerData.fontFamily || "Arial"
            font.pixelSize: displayData.fontSize || 36
            readOnly: true
            Accessible.readOnly: true
            Accessible.editable: false
            activeFocusOnPress: false
            selectByMouse: false
            wrapMode: visualLayer.layerData.wrap === false ? TextEdit.NoWrap : TextEdit.Wrap
            clip: visualLayer.layerData.clipText !== false
            verticalAlignment: visualLayer.layerData.verticalAlignment === "center"
                               ? TextEdit.AlignVCenter : visualLayer.layerData.verticalAlignment
                                 === "bottom" ? TextEdit.AlignBottom : TextEdit.AlignTop
            horizontalAlignment: visualLayer.layerData.alignment === "center"
                                 ? TextEdit.AlignHCenter : visualLayer.layerData.alignment
                                   === "right" ? TextEdit.AlignRight :
                                                 visualLayer.layerData.alignment === "justify"
                                                 ? TextEdit.AlignJustify : TextEdit.AlignLeft

            Component.onCompleted: Qt.callLater(restoreFormatting)

            Connections {
                function onLayerDataChanged() {
                    Qt.callLater(renderedText.restoreFormatting);
                }

                target: visualLayer
            }
        }
    }

    Component {
        id: media

        MediaLayer {
            sourceUrl: visualLayer.layerData.source || ""
            kind: visualLayer.layerData.kind
            fit: visualLayer.layerData.fit || "contain"
            playing: visualLayer.playing && visualLayer.visible
            audible: visualLayer.audible && !visualLayer.layerData.muted
            looping: visualLayer.layerData.loop !== false
            decodeAudio: visualLayer.decodeAudio
            videoEnabled: visualLayer.videoEnabled
            showErrors: visualLayer.showErrors

            onMediaProblem: message => visualLayer.mediaProblem(message)
        }
    }
}
