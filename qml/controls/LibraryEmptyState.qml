pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: emptyState

    property string title: ""
    property string description: ""
    property string iconName: "image"

    implicitWidth: 320
    implicitHeight: content.implicitHeight

    Column {
        id: content

        width: parent.width
        spacing: 12

        StudioIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: emptyState.iconName
            width: 28
            height: 28
            tone: LibraryStyle.tone("#999999")
        }

        Text {
            renderType: Text.QtRendering
            width: parent.width
            text: emptyState.title
            color: LibraryStyle.text
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 14
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Text {
            renderType: Text.QtRendering
            width: parent.width
            text: emptyState.description
            color: LibraryStyle.muted
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            lineHeight: 1.4
        }
    }
}
