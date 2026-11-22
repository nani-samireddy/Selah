pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel

    required property var controller

    signal controlsRequested

    implicitHeight: 134
    color: LibraryStyle.panel
    radius: 10
    border.color: LibraryStyle.divider

    RowLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            Text {
                text: "SPEAKER NOTES"
                font.pixelSize: 10
                font.letterSpacing: 1
                color: LibraryStyle.muted
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Text {
                    width: parent.width
                    text: panel.controller.liveNotes || "No notes for this slide"
                    textFormat: Text.PlainText
                    color: LibraryStyle.text
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
            }

            Text {
                visible: panel.controller.stageMessage.length > 0
                text: "Stage: " + panel.controller.stageMessage
                textFormat: Text.PlainText
                color: LibraryStyle.dark ? "#D4B993" : "#7D572A"
                font.pixelSize: 11
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        Rectangle {
            color: LibraryStyle.divider
            Layout.preferredWidth: 1
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 92
            spacing: 7

            Text {
                text: "STAGE TIMER"
                font.pixelSize: 9
                font.letterSpacing: 1
                color: LibraryStyle.muted
            }

            Text {
                text: panel.controller.countdown
                font.pixelSize: 25
                color: LibraryStyle.text
            }

            RowLayout {
                StudioButton {
                    iconName: "monitor-play"
                    hint: "Stage preview"
                    enabled: panel.controller.liveCount > 0

                    onClicked: panel.controller.preview("stage")
                }

                StudioButton {
                    iconName: "settings-2"
                    hint: "Stage controls"

                    onClicked: panel.controlsRequested()
                }
            }
        }
    }
}
