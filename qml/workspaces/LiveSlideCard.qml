pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: card

    required property var controller
    property bool next: false
    property bool available: next ? controller.liveIndex + 1 < controller.liveCount :
                                    controller.liveCount > 0

    spacing: 10

    RowLayout {
        Text {
            text: card.next ? "UP NEXT" : "LIVE"
            color: card.next ? LibraryStyle.tone("#A3A3A3") : "#B5D8C1"
            font.pixelSize: 10
            font.letterSpacing: 1.2
        }

        Text {
            text: card.available ? (card.next ? card.controller.nextTitle :
                                                card.controller.liveTitle) : card.next
                                   ? "End of presentation" : "Nothing live"
            color: LibraryStyle.tone("#DDDDDD")
            font.pixelSize: 12
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    Rectangle {
        id: frame

        Layout.fillWidth: true
        Layout.preferredHeight: width * 9 / 16
        color: LibraryStyle.tone("#090909")
        radius: 8
        border.color: !card.next && card.available ? "#577562" : LibraryStyle.tone("#373737")
        clip: true

        SlideRenderer {
            visible: card.available
            scale: frame.width / 1920
            transformOrigin: Item.TopLeft
            elements: card.next ? card.controller.nextElements : card.controller.liveElements
            backgroundColor: card.next ? card.controller.nextBackground :
                                         card.controller.liveBackground
            ambientOpacity: card.next ? card.controller.nextAmbientOpacity :
                                        card.controller.liveAmbientOpacity
            videoEnabled: false
        }

        Text {
            visible: !card.available
            anchors.centerIn: parent
            text: card.next ? "End of presentation" : "No presentation is live"
            color: LibraryStyle.tone("#777777")
            font.pixelSize: 13
        }
    }
}
