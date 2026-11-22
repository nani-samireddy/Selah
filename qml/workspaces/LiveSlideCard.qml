pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: card

    required property var controller
    property bool next: false
    property real previewHeightLimit: 10000
    property bool available: next ? controller.liveIndex + 1 < controller.liveCount :
                                    controller.hasAudienceContent

    spacing: 10

    RowLayout {
        Text {
            text: card.next ? "UP NEXT" : "AUDIENCE"
            color: card.next ? LibraryStyle.muted : LibraryStyle.dark ? "#B5D8C1" : "#306044"
            font.pixelSize: 10
            font.letterSpacing: 1.2
        }

        Text {
            text: card.available ? (card.next ? card.controller.nextTitle :
                                                card.controller.audienceTitle) : card.next
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
        Layout.maximumWidth: card.previewHeightLimit * 16 / 9
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredHeight: width * 9 / 16
        color: LibraryStyle.tone("#090909")
        radius: 8
        border.color: !card.next && card.available ? "#577562" : LibraryStyle.tone("#373737")
        clip: true

        SlideRenderer {
            visible: card.available
            scale: frame.width / 1920
            transformOrigin: Item.TopLeft
            elements: card.next ? card.controller.nextElements : card.controller.audienceElements
            backgroundColor: card.next ? card.controller.nextBackground :
                                         card.controller.audienceBackground
            ambientOpacity: card.next ? card.controller.nextAmbientOpacity :
                                        card.controller.audienceAmbientOpacity
            clearText: !card.next && card.controller.textCleared
            videoEnabled: false
        }

        Rectangle {
            anchors.fill: parent
            color: "#000000"
            visible: !card.next && card.controller.audienceBlack
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
