pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    required property var service
    property bool speaker: false

    width: Math.min(700, parent.width - 60)
    height: Math.min(760, parent.height - 40)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 24
    modal: true
    focus: true

    background: SquircleSurface {
        color: LibraryStyle.tone("#191919")
        radius: 12
        border.color: LibraryStyle.tone("#3B3B3B")
    }

    onOpened: service.refreshInterfaces()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Text {
                text: "Remote control"
                color: LibraryStyle.tone("#EEEEEE")
                font.pixelSize: 22
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        Text {
            text: panel.service.status
            color: panel.service.running ? "#ACCFB5" : LibraryStyle.tone("#999999")
            font.pixelSize: 12
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryCombo {
                id: network

                model: panel.service.interfaces
                textRole: "label"
                valueRole: "address"
                visible: !panel.service.running
                Layout.fillWidth: true
            }

            Text {
                visible: panel.service.running
                text: panel.service.address
                color: LibraryStyle.tone("#BBBBBB")
                font.pixelSize: 13
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Refresh"
                visible: !panel.service.running

                onClicked: panel.service.refreshInterfaces()
            }

            StudioButton {
                text: panel.service.running ? "Stop remote" : "Start remote"
                accented: !panel.service.running

                onClicked: panel.service.running ? panel.service.stop() : panel.service.start(
                                                       network.currentValue || "127.0.0.1")
            }
        }

        Text {
            text: "Use a trusted local Wi-Fi network. HTTP pairing stays on your network. Allow local network access if your system asks."
            color: LibraryStyle.tone("#909090")
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: panel.availableWidth - 12
                spacing: 16
                visible: panel.service.running

                RowLayout {
                    Layout.fillWidth: true

                    StudioButton {
                        text: "Controller"
                        checked: !panel.speaker

                        onClicked: panel.speaker = false
                    }

                    StudioButton {
                        text: "Speaker · read only"
                        checked: panel.speaker

                        onClicked: panel.speaker = true
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    StudioButton {
                        text: "New codes"
                        hint: "Disconnect all paired devices and replace both codes"

                        onClicked: panel.service.rotateCodes()
                    }
                }

                RowLayout {
                    spacing: 24

                    Rectangle {
                        color: "white"
                        radius: 10
                        Layout.preferredWidth: 192
                        Layout.preferredHeight: 192

                        Image {
                            anchors.fill: parent
                            anchors.margins: 8
                            source: panel.speaker ? panel.service.speakerQr :
                                                    panel.service.controllerQr
                            fillMode: Image.PreserveAspectFit
                            smooth: false
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            text: panel.speaker ? "Speaker pairing code" : "Controller pairing code"
                            color: LibraryStyle.tone("#A0A0A0")
                            font.pixelSize: 12
                        }

                        Text {
                            text: panel.speaker ? panel.service.speakerCode :
                                                  panel.service.controllerCode
                            color: LibraryStyle.tone("#EEEEEE")
                            font.pixelSize: 32
                            font.letterSpacing: 5
                        }

                        Text {
                            text: "Scan the QR code, or open this address and enter the code:"
                            color: LibraryStyle.tone("#999999")
                            font.pixelSize: 12
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }

                        TextEdit {
                            text: panel.service.address
                            readOnly: true
                            selectByMouse: true
                            color: LibraryStyle.tone("#CCCCCC")
                            font.pixelSize: 13
                            wrapMode: TextEdit.WrapAnywhere
                            Layout.fillWidth: true
                        }
                    }
                }

                Text {
                    text: panel.speaker
                          ? "Speakers can see live and next text, translations, notes, messages and countdown. They cannot change slides." :
                            "Controllers can move through the live snapshot and blackout the audience. Previews are still images; video and audio stay on the computer."
                    color: LibraryStyle.tone("#999999")
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: LibraryStyle.tone("#333333")
                }

                Text {
                    text: "Paired devices · " + panel.service.clients.length + " / 24"
                    color: LibraryStyle.tone("#DDDDDD")
                    font.pixelSize: 14
                }

                Text {
                    text: "No devices paired yet"
                    visible: panel.service.clients.length === 0
                    color: LibraryStyle.tone("#777777")
                    font.pixelSize: 12
                }

                Repeater {
                    model: panel.service.clients

                    RowLayout {
                        id: clientRow

                        required property var modelData

                        Layout.fillWidth: true

                        Text {
                            text: clientRow.modelData.name
                            textFormat: Text.PlainText
                            color: LibraryStyle.tone("#DDDDDD")
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: clientRow.modelData.role
                            color: LibraryStyle.tone("#888888")
                            font.pixelSize: 12
                        }

                        StudioButton {
                            text: "Disconnect"

                            onClicked: panel.service.revoke(clientRow.modelData.id)
                        }
                    }
                }
            }
        }
    }
}
