pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes

// Continuous, boxier corners shared by cards, dialogs and floating surfaces.
// Keep children outside the Shape so content remains sharp and interactive.
Item {
    id: surface

    property color color: "transparent"
    property real radius: 12
    property alias fillGradient: outline.fillGradient
    property alias border: strokeStyle.border
    readonly property real inset: border.color.a > 0 ? border.width / 2 : 0
    readonly property real corner: Math.max(0, Math.min(radius * 1.5, (width - 2 * inset) / 2, (
                                                            height - 2 * inset) / 2))
    readonly property real edgeLeft: inset
    readonly property real edgeTop: inset
    readonly property real edgeRight: width - inset
    readonly property real edgeBottom: height - inset
    readonly property real handle: corner * 0.9

    // Preserve Rectangle's native grouped border API for all existing callers.
    Rectangle {
        id: strokeStyle

        visible: false
        border.color: "transparent"
        border.width: 1
    }

    Shape {
        anchors.fill: parent
        z: -1
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            id: outline

            fillColor: surface.color
            strokeColor: surface.border.color
            strokeWidth: surface.border.color.a > 0 ? surface.border.width : -1
            startX: surface.edgeLeft + surface.corner
            startY: surface.edgeTop

            PathLine {
                x: surface.edgeRight - surface.corner
                y: surface.edgeTop
            }

            PathCubic {
                x: surface.edgeRight
                y: surface.edgeTop + surface.corner
                control1X: surface.edgeRight - surface.corner + surface.handle
                control1Y: surface.edgeTop
                control2X: surface.edgeRight
                control2Y: surface.edgeTop + surface.corner - surface.handle
            }

            PathLine {
                x: surface.edgeRight
                y: surface.edgeBottom - surface.corner
            }

            PathCubic {
                x: surface.edgeRight - surface.corner
                y: surface.edgeBottom
                control1X: surface.edgeRight
                control1Y: surface.edgeBottom - surface.corner + surface.handle
                control2X: surface.edgeRight - surface.corner + surface.handle
                control2Y: surface.edgeBottom
            }

            PathLine {
                x: surface.edgeLeft + surface.corner
                y: surface.edgeBottom
            }

            PathCubic {
                x: surface.edgeLeft
                y: surface.edgeBottom - surface.corner
                control1X: surface.edgeLeft + surface.corner - surface.handle
                control1Y: surface.edgeBottom
                control2X: surface.edgeLeft
                control2Y: surface.edgeBottom - surface.corner + surface.handle
            }

            PathLine {
                x: surface.edgeLeft
                y: surface.edgeTop + surface.corner
            }

            PathCubic {
                x: surface.edgeLeft + surface.corner
                y: surface.edgeTop
                control1X: surface.edgeLeft
                control1Y: surface.edgeTop + surface.corner - surface.handle
                control2X: surface.edgeLeft + surface.corner - surface.handle
                control2Y: surface.edgeTop
            }
        }
    }
}
