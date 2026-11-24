pragma ComponentBehavior: Bound
import QtQuick

// Gradients redraw only when their settings change; slide transitions use GPU textures.
Item {
    id: fill

    property var definition: ({})
    property color baseColor: "#252A3A"
    readonly property string kind: definition.type || "color"

    Rectangle {
        anchors.fill: parent
        visible: fill.kind === "color" || fill.kind === "image"
        color: Qt.rgba(fill.baseColor.r, fill.baseColor.g, fill.baseColor.b, 1)
    }

    Canvas {
        id: gradient

        anchors.fill: parent
        visible: fill.kind === "linear" || fill.kind === "radial"

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onVisibleChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            if (!visible || width <= 0 || height <= 0)
                return;
            const radians = (fill.definition.angle || 0) * Math.PI / 180;
            const dx = Math.cos(radians), dy = Math.sin(radians);
            const length = Math.abs(dx) * width + Math.abs(dy) * height;
            const paint = fill.kind === "radial" ? ctx.createRadialGradient(width / 2, height / 2, 0,
                                                                            width / 2, height / 2,
                                                                            Math.sqrt(width * width
                                                                                      + height
                                                                                      * height)
                                                                            / 2) : ctx.createLinearGradient(
                                                       width / 2 - dx * length / 2, height / 2 - dy * length
                                                       / 2, width / 2 + dx * length / 2, height / 2
                                                       + dy * length / 2);
            for (const stop of fill.definition.stops || [])
                paint.addColorStop(stop.position, stop.color);
            ctx.fillStyle = paint;
            ctx.fillRect(0, 0, width, height);
        }

        Connections {
            function onDefinitionChanged() {
                gradient.requestPaint();
            }

            target: fill
        }
    }

    Image {
        anchors.fill: parent
        visible: fill.kind === "image"
        source: visible ? fill.definition.source || "" : ""
        asynchronous: true
        sourceSize.width: 1920
        sourceSize.height: 0
        fillMode: fill.definition.fit === "contain" ? Image.PreserveAspectFit : fill.definition.fit
                                                      === "stretch" ? Image.Stretch :
                                                                      Image.PreserveAspectCrop
    }
}
