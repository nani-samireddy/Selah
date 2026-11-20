pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: liveControls

    required property var programView

    Layout.preferredWidth: 245
    Layout.maximumWidth: 270
    Layout.minimumWidth: 210
    Layout.fillHeight: true
    contentWidth: availableWidth
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
        width: liveControls.availableWidth
        spacing: 12

        Text {
            text: liveControls.programView.store.live ? "LIVE PROGRAM" : "LIVE OUTPUT"
            color: liveControls.programView.store.live ? "#ABD3BC" : "#888888"
            font.pixelSize: 10
            font.letterSpacing: 1
        }

        Repeater {
            model: 2

            ColumnLayout {
                id: liveCard

                required property int index

                Layout.fillWidth: true
                spacing: 7

                Text {
                    text: liveCard.index === 0 ? "Current" : "Next"
                    color: "#999999"
                    font.pixelSize: 11
                }

                Rectangle {
                    id: liveFrame

                    Layout.fillWidth: true
                    Layout.preferredHeight: width * 9 / 16
                    color: "#111111"
                    border.color: "#353535"
                    radius: 5
                    clip: true

                    SlideRenderer {
                        width: 1920
                        height: 1080
                        scale: liveFrame.width / 1920
                        transformOrigin: Item.TopLeft
                        elements: liveCard.index === 0
                                  ? liveControls.programView.controller.liveElements :
                                    liveControls.programView.controller.nextElements
                        backgroundColor: liveCard.index === 0
                                         ? liveControls.programView.controller.liveBackground :
                                           liveControls.programView.controller.nextBackground
                        ambientOpacity: liveCard.index === 0
                                        ? liveControls.programView.controller.liveAmbientOpacity :
                                          liveControls.programView.controller.nextAmbientOpacity
                        videoEnabled: false
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: (liveCard.index === 0 ? liveControls.programView.controller.liveTitle :
                                                  liveControls.programView.controller.nextTitle)
                          || "No slide"
                    font.pixelSize: 11
                    color: "#AAAAAA"
                    elide: Text.ElideRight
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: liveControls.programView.store.live ? "Slide " + (
                                                            liveControls.programView.controller.liveIndex
                                                            + 1) + " / "
                                                        + liveControls.programView.controller.liveCount :
                                                        "Take a program slide live to start."
            font.pixelSize: 11
            color: "#999999"
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true

            StudioButton {
                text: "Previous"
                Layout.fillWidth: true
                enabled: liveControls.programView.store.live
                         && liveControls.programView.controller.liveIndex > 0

                onClicked: liveControls.programView.navigate(-1)
            }

            StudioButton {
                text: "Next"
                Layout.fillWidth: true
                enabled: liveControls.programView.store.live
                         && liveControls.programView.controller.liveIndex
                         < liveControls.programView.controller.liveCount - 1

                onClicked: liveControls.programView.navigate(1)
            }
        }

        Text {
            text: "← Previous   ·   → / Space Next\nHome / End   ·   First / last slide"
            color: "#777777"
            font.pixelSize: 10
            lineHeight: 1.5
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#333333"
        }

        StudioButton {
            text: liveControls.programView.controller.audienceBlack ? "Restore audience" :
                                                                      "Blackout audience"
            checked: liveControls.programView.controller.audienceBlack
            Layout.fillWidth: true

            onClicked: liveControls.programView.controller.blackout("audience",
                                                                    !liveControls.programView.controller.audienceBlack)
        }

        StudioButton {
            text: "Audience preview"
            Layout.fillWidth: true
            enabled: liveControls.programView.controller.liveCount > 0

            onClicked: liveControls.programView.controller.preview("audience")
        }

        StudioButton {
            text: "Stage preview"
            Layout.fillWidth: true
            enabled: liveControls.programView.controller.liveCount > 0

            onClicked: liveControls.programView.controller.preview("stage")
        }

        StudioButton {
            text: "Output settings"
            Layout.fillWidth: true

            onClicked: liveControls.programView.outputsRequested()
        }

        Text {
            Layout.fillWidth: true
            text: "Library and draft edits stay off the live service until taken live again."
            color: "#888888"
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }
    }
}
