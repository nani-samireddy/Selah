pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects

Item {
    id: icon

    property string name: ""
    property alias source: iconImage.source
    property color tone: LibraryStyle.tone("#A3A3A3")

    implicitWidth: 14
    implicitHeight: 14

    Image {
        id: iconImage

        anchors.fill: parent
        source: icon.name.length ? "../assets/icons/" + icon.name + ".svg" : ""
        sourceSize.width: Math.ceil(width * Screen.devicePixelRatio)
        sourceSize.height: Math.ceil(height * Screen.devicePixelRatio)
        smooth: true
        layer.enabled: true

        layer.effect: MultiEffect {
            colorization: 1
            colorizationColor: icon.tone
        }
    }
}
