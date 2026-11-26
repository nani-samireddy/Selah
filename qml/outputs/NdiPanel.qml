pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: panel

    required property var sender

    function start(diagnostic) {
        sender.start(sourceName.text, resolution.currentIndex === 0 ? 1280 : 1920,
                     resolution.currentIndex === 0 ? 720 : 1080, frameRate.currentIndex === 0 ? 30 :
                                                                                                60, mode.currentIndex
                     === 1, diagnostic);
    }

    width: Math.min(620, parent.width - 60)
    height: Math.min(620, parent.height - 40)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 20
    modal: true
    focus: true

    background: SquircleSurface {
        color: LibraryStyle.tone("#191919")
        radius: 12
        border.color: LibraryStyle.tone("#3B3B3B")
    }

    onOpened: if (sender.busy) {
                  resolution.currentIndex = sender.frameWidth === 1920 ? 1 : 0;
                  frameRate.currentIndex = sender.targetFps === 60 ? 1 : 0;
                  mode.currentIndex = sender.overlayOnly ? 1 : 0;
              }

    FileDialog {
        id: runtimeDialog

        title: "Choose the installed NDI runtime library"

        onAccepted: panel.sender.selectRuntime(selectedFile)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "NDI broadcast prototype"
                color: LibraryStyle.tone("#EEEEEE")
                font.pixelSize: 20
                Layout.fillWidth: true
            }

            StudioButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        Text {
            text: "Full slides or transparent overlays from the live presentation."
            color: LibraryStyle.tone("#999999")
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Text {
            text: "Transparent canvas keeps text, images, videos, and graphics visible. Uncovered areas stay transparent; full-canvas media still covers the canvas."
            color: LibraryStyle.tone("#999999")
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            enabled: !panel.sender.busy
            spacing: 10

            TextField {
                id: sourceName

                text: "Selah Broadcast"
                placeholderText: "NDI source name"
                color: LibraryStyle.tone("#DDDDDD")
                selectByMouse: true
                font.pixelSize: 12
                padding: 10
                Layout.fillWidth: true

                background: Rectangle {
                    color: LibraryStyle.tone("#252525")
                    radius: 5
                    border.color: LibraryStyle.tone("#3D3D3D")
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Choice {
                    id: resolution

                    Layout.fillWidth: true
                    model: ["1280 × 720", "1920 × 1080"]
                }

                Choice {
                    id: frameRate

                    Layout.fillWidth: true
                    model: ["30 fps", "60 fps"]
                }

                Choice {
                    id: mode

                    Layout.fillWidth: true
                    model: ["Full slide", "Transparent canvas"]
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: panel.sender.runtimePath || "NDI runtime not installed"
                    elide: Text.ElideMiddle
                    color: LibraryStyle.tone("#858585")
                    font.pixelSize: 11
                    Layout.fillWidth: true
                }

                StudioButton {
                    text: "Locate runtime"

                    onClicked: runtimeDialog.open()
                }
            }
        }

        Text {
            text: panel.sender.sdkEnabled ? "SDK support enabled in this build" :
                                            "SDK headers unavailable in this build. Capture-only testing is available."
            color: LibraryStyle.tone("#AEAEAE")
            font.pixelSize: 11
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true

            StudioButton {
                text: "Start NDI"
                accented: true
                enabled: panel.sender.sdkEnabled && !panel.sender.busy

                onClicked: panel.start(false)
            }

            StudioButton {
                text: "Test capture"
                enabled: !panel.sender.busy

                onClicked: panel.start(true)
            }

            StudioButton {
                text: "Stop"
                enabled: panel.sender.busy

                onClicked: panel.sender.stop()
            }

            StudioButton {
                text: panel.sender.blackout ? "Restore" : "Blackout"
                checked: panel.sender.blackout
                enabled: panel.sender.active

                onClicked: panel.sender.blackout = !panel.sender.blackout
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: LibraryStyle.tone("#353535")
        }

        Text {
            text: panel.sender.status
            color: LibraryStyle.tone("#DDDDDD")
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        GridLayout {
            columns: 2
            columnSpacing: 36
            rowSpacing: 12
            Layout.fillWidth: true

            Text {
                text: "Captured frames"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.framesCaptured
                color: LibraryStyle.tone("#DDDDDD")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.diagnostic ? "Processed locally" : "Sent to NDI SDK"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.framesProcessed
                color: LibraryStyle.tone("#DDDDDD")
                font.pixelSize: 12
            }

            Text {
                text: "Capture rate / render time"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.captureFps.toFixed(1) + " fps  /  "
                      + panel.sender.averageRenderMs.toFixed(1) + " ms"
                color: LibraryStyle.tone("#DDDDDD")
                font.pixelSize: 12
            }

            Text {
                text: "Late capture / queue drops"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.lateCaptureFrames + "  /  " + panel.sender.droppedFrames
                color: LibraryStyle.tone("#DDDDDD")
                font.pixelSize: 12
            }

            Text {
                text: "NDI receivers"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.connections
                color: LibraryStyle.tone("#DDDDDD")
                font.pixelSize: 12
            }

            Text {
                text: "Graphics backend"
                color: LibraryStyle.tone("#929292")
                font.pixelSize: 12
            }

            Text {
                text: panel.sender.backendName || "Not started"
                color: LibraryStyle.tone("#DDDDDD")
                font.pixelSize: 12
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Text {
            text: "Video only in this prototype. NDI audio and receiver validation are pending."
            color: LibraryStyle.tone("#8B8B8B")
            font.pixelSize: 11
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Text {
            text: "<a href='https://ndi.video/'>NDI®</a> · <a href='https://ndi.video/for-developers/ndi-sdk/download/'>Official SDK / runtime setup</a>"
            color: LibraryStyle.tone("#999999")
            linkColor: LibraryStyle.tone("#BBBBBB")
            font.pixelSize: 11

            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    component Choice: ComboBox {
        id: choice

        implicitHeight: 34

        background: Rectangle {
            color: LibraryStyle.tone("#252525")
            radius: 5
            border.color: choice.visualFocus ? LibraryStyle.tone("#888888") : LibraryStyle.tone(
                                                   "#3D3D3D")
        }
        indicator: StudioIcon {
            name: "chevron-down"
            x: choice.width - 23
            y: (choice.height - height) / 2
        }
        contentItem: Text {
            text: choice.displayText
            color: LibraryStyle.tone("#CCCCCC")
            font.pixelSize: 11
            verticalAlignment: Text.AlignVCenter
            leftPadding: 9
            rightPadding: 26
            elide: Text.ElideRight
        }
    }
}
