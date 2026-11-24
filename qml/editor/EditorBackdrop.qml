pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

// Capture only the workspace, never its floating panels. All panels share one blur.
Item {
    id: backdrop

    required property Item scene
    property bool active: true
    readonly property bool available: active && GraphicsInfo.api !== GraphicsInfo.Software
    readonly property var texture: blurredTexture

    ShaderEffectSource {
        id: sceneTexture

        anchors.fill: parent
        sourceItem: backdrop.available ? backdrop.scene : null
        textureSize: Qt.size(Math.max(1, Math.ceil(width / 2)), Math.max(1, Math.ceil(height / 2)))
        live: backdrop.available
        visible: false
    }

    MultiEffect {
        id: softenedScene

        anchors.fill: parent
        source: sceneTexture
        blurEnabled: true
        blurMax: 40
        blur: 1.0
        autoPaddingEnabled: false
        visible: false
    }

    ShaderEffectSource {
        id: blurredTexture

        anchors.fill: parent
        sourceItem: backdrop.available ? softenedScene : null
        textureSize: sceneTexture.textureSize
        live: backdrop.available
        visible: false
    }
}
