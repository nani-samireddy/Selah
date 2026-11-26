pragma ComponentBehavior: Bound
import QtQuick

import QtQuick.Effects

SquircleSurface {
    id: panel

    property var backdropSource: null
    property rect backdropRect: Qt.rect(0, 0, width, height)
    readonly property color surfaceColor: LibraryStyle.tone("#171717")
    property real surfaceOpacity: LibraryStyle.dark ? 0.75 : 0.82
    property real backdropSaturation: 1.5

    // Tint only the surface; labels, icons and input controls retain full contrast.
    color: backdropSource ? "transparent" : Qt.rgba(surfaceColor.r, surfaceColor.g, surfaceColor.b,
                                                    surfaceOpacity)

    border.color: LibraryStyle.dark ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(0, 0, 0, 0.12)
    radius: 9
    layer.enabled: true

    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowOpacity: 0.3
        shadowBlur: 0.5
        shadowVerticalOffset: 8
    }

    ShaderEffect {
        property var backdrop: panel.backdropSource
        property vector4d sampleRect: panel.backdropSource ? Qt.vector4d(panel.backdropRect.x
                                                                         / panel.backdropSource.width,
                                                                         panel.backdropRect.y
                                                                         / panel.backdropSource.height,
                                                                         panel.backdropRect.width
                                                                         / panel.backdropSource.width,
                                                                         panel.backdropRect.height
                                                                         / panel.backdropSource.height) :
                                                             Qt.vector4d(0, 0, 1, 1)
        property vector2d panelSize: Qt.vector2d(width, height)
        property color tint: Qt.rgba(panel.surfaceColor.r, panel.surfaceColor.g,
                                     panel.surfaceColor.b, panel.surfaceOpacity)
        property real saturation: panel.backdropSaturation
        property real cornerRadius: Math.max(0, panel.corner - 1)

        anchors.fill: parent
        anchors.margins: 1
        visible: !!panel.backdropSource
        fragmentShader: "qrc:/shaders/panel-backdrop.frag.qsb"
    }

    // A floating panel owns its surface, including gaps between controls.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons

        onWheel: event => event.accepted = true
    }
}
