pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Uses static video placeholders to avoid decoding every slide in the deck.
Item {
    id: slideStrip

    required property var host
    required property Item canvas
    property bool compact: false
    readonly property real previewWidth: compact ? 56 : 126
    readonly property real previewPadding: compact ? 2 : 4
    readonly property real previewHeight: (previewWidth - 2 * previewPadding) * 9 / 16 + 2
                                          * previewPadding

    function revealCurrent() {
        const start = host.doc.currentSlide * (previewWidth + thumbnailRow.spacing);
        const end = start + previewWidth;
        const desired = start < scroller.contentX ? start : end > scroller.contentX
                                                    + scroller.width ? end - scroller.width :
                                                                       scroller.contentX;
        scroller.contentX = Math.max(0, Math.min(desired, scroller.contentWidth - scroller.width));
    }

    objectName: "editorSlideStrip"
    visible: !host.templateMode
    height: !host.templateMode ? (compact ? previewHeight : 32 + previewHeight + 22) : 0

    Connections {
        function onChanged() {
            Qt.callLater(slideStrip.revealCurrent);
        }

        target: slideStrip.host.doc
    }

    RowLayout {
        visible: !slideStrip.compact
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
            enabled: !slideStrip.host.templateMode

            onClicked: if (!slideStrip.host.templateMode)
                           slideStrip.host.doc.addSlide()
        }
    }

    Flickable {
        id: scroller

        contentWidth: thumbnailRow.width
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: slideStrip.compact ? 0 : 32
            bottom: parent.bottom
        }

        WheelHandler {
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

            onWheel: event => {
                const delta = event.pixelDelta.x || event.pixelDelta.y || event.angleDelta.y / 2;
                scroller.contentX = Math.max(0, Math.min(scroller.contentX - delta,
                                                         scroller.contentWidth - scroller.width));
                event.accepted = true;
            }
        }

        Row {
            id: thumbnailRow

            spacing: slideStrip.compact ? 6 : 14

            Repeater {
                model: slideStrip.host.doc.slides

                delegate: Item {
                    id: thumbnail

                    required property var modelData
                    required property int index

                    width: slideStrip.previewWidth
                    height: slideStrip.previewHeight + (slideStrip.compact ? 0 : 22)
                    ToolTip.visible: slideStrip.compact && thumbMouse.containsMouse
                    ToolTip.text: String(thumbnail.index + 1) + " · " + thumbnail.modelData.name
                    ToolTip.delay: 500

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
                                showTransparencyGrid: true
                                elements: thumbnail.modelData.elements
                                backgroundFill: thumbnail.modelData.backgroundFill || ({})
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
                        visible: !slideStrip.compact
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
                        id: thumbMouse

                        hoverEnabled: slideStrip.compact
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            if (slideStrip.host.activeTextEditor)
                                slideStrip.host.activeTextEditor.finish();
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
