pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

// Uses static video placeholders to avoid decoding every slide in the deck.
Item {
    id: slideStrip

    required property var host
    required property Item canvas
    readonly property real previewWidth: 126
    readonly property real previewPadding: 4
    readonly property real previewHeight: (previewWidth - 2 * previewPadding) * 9 / 16 + 2
                                          * previewPadding

    height: 32 + previewHeight + 22

    RowLayout {
        height: 20

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        StudioMicroLabel {
            text: slideStrip.host.templateMode ? "TEMPLATE PREVIEWS" : "SLIDES"
            Layout.fillWidth: true
        }

        StudioButton {
            text: "Add slide"
            iconName: "plus"
            quiet: true
            implicitHeight: 25
            hint: "Create a new slide"

            onClicked: slideStrip.host.doc.addSlide()
        }
    }

    Flickable {
        contentWidth: thumbnailRow.width
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 32
            bottom: parent.bottom
        }

        Row {
            id: thumbnailRow

            spacing: 14

            Repeater {
                model: slideStrip.host.doc.slides

                delegate: Item {
                    id: thumbnail

                    required property var modelData
                    required property int index

                    width: slideStrip.previewWidth
                    height: slideStrip.previewHeight + 22

                    Rectangle {
                        width: thumbnail.width
                        height: slideStrip.previewHeight
                        radius: 4
                        color: "transparent"
                        border.color: slideStrip.host.doc.currentSlide === thumbnail.index
                                      ? LibraryStyle.tone("#DDDDDD") : LibraryStyle.tone("#373737")
                        border.width: slideStrip.host.doc.currentSlide === thumbnail.index ? 2 : 1

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: slideStrip.previewPadding
                            color: thumbnail.modelData.background
                            clip: true

                            SlideRenderer {
                                elements: thumbnail.modelData.elements
                                backgroundColor: thumbnail.modelData.background
                                ambientOpacity: thumbnail.modelData.ambientOpacity === undefined ? (
                                                                                                       thumbnail.index
                                                                                                       === 0 ? 0.85 :
                                                                                                               0.5) : thumbnail.modelData.ambientOpacity
                                videoEnabled: false
                                scale: parent.width / 1920
                                transformOrigin: Item.TopLeft
                            }
                        }
                    }

                    Text {
                        y: slideStrip.previewHeight + 5
                        width: thumbnail.width
                        elide: Text.ElideRight
                        text: String(thumbnail.index + 1).padStart(2, "0") + "  "
                              + thumbnail.modelData.name
                        color: slideStrip.host.doc.currentSlide === thumbnail.index
                               ? LibraryStyle.tone("#D6D6D6") : LibraryStyle.tone("#777777")
                        font.pixelSize: 10
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            slideStrip.canvas.forceActiveFocus();
                            slideStrip.host.doc.currentSlide = thumbnail.index;
                            slideStrip.host.panX = 0;
                            slideStrip.host.panY = 0;
                        }
                    }
                }
            }
        }
    }
}
