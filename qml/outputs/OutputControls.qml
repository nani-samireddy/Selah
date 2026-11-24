pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: controls

    required property var controller
    property string section: "displays"

    signal remoteRequested
    signal broadcastRequested

    objectName: "presentOutputControls"
    spacing: 16

    RowLayout {
        Layout.fillWidth: true

        Rectangle {
            implicitWidth: tabs.implicitWidth + 8
            implicitHeight: 36
            radius: 8
            color: LibraryStyle.panel
            border.color: LibraryStyle.border

            Row {
                id: tabs

                x: 4
                y: 4
                spacing: 2

                Repeater {
                    model: [
                        {
                            key: "displays",
                            label: "Displays"
                        },
                        {
                            key: "speaker",
                            label: "Speaker tools"
                        }
                    ]

                    LibraryButton {
                        required property var modelData

                        objectName: "outputTab_" + modelData.key
                        text: modelData.label
                        selected: controls.section === modelData.key
                        quiet: !selected

                        onClicked: controls.section = modelData.key
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        objectName: "outputDisplaySettings"
        visible: controls.section === "displays"
        Layout.fillWidth: true
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            OutputDisplayCard {
                controller: controls.controller
                role: "audience"
            }

            OutputDisplayCard {
                controller: controls.controller
                role: "stage"
            }
        }

        LibraryCaption {
            text: "Connections"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            OutputGroup {
                Layout.preferredWidth: 1

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Text {
                            text: "NDI output"
                            color: LibraryStyle.text
                            font.pixelSize: 14
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "Video over the local network"
                            color: LibraryStyle.muted
                            font.pixelSize: 11
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    LibraryButton {
                        text: "Configure"

                        onClicked: {
                            controls.broadcastRequested();
                        }
                    }
                }
            }

            OutputGroup {
                Layout.preferredWidth: 1

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Text {
                            text: "Remote control"
                            color: LibraryStyle.text
                            font.pixelSize: 14
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "Control from a phone or tablet"
                            color: LibraryStyle.muted
                            font.pixelSize: 11
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    LibraryButton {
                        text: "Open"

                        onClicked: controls.remoteRequested()
                    }
                }
            }
        }
    }

    OutputSpeakerControls {
        visible: controls.section === "speaker"
        Layout.fillWidth: true
        controller: controls.controller
    }

    Text {
        text: controls.controller.status
        color: LibraryStyle.muted
        font.pixelSize: 11
        Layout.fillWidth: true
        wrapMode: Text.Wrap
    }
}
