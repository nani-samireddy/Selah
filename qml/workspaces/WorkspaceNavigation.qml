pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: navigation

    required property var host
    readonly property bool fullscreen: host.visibility === Window.FullScreen
    readonly property real captionLeft: !fullscreen && Qt.platform.os === "osx" ? 90 : 0
    readonly property real captionRight: !fullscreen && Qt.platform.os === "windows" ? 150 : 0

    implicitHeight: 66
    color: LibraryStyle.tone("#191919")

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: LibraryStyle.tone("#303030")
    }

    // Behind the controls: toolbar buttons keep their normal pointer handling.
    MouseArea {
        objectName: "windowDragArea"
        anchors.fill: parent
        anchors.leftMargin: navigation.captionLeft
        anchors.rightMargin: navigation.captionRight
        enabled: !navigation.fullscreen
        acceptedButtons: Qt.LeftButton

        onPressed: mouse => {
            if (mouse.button === Qt.LeftButton)
                navigation.host.startSystemMove();
        }
        onDoubleClicked: {
            if (navigation.host.visibility === Window.Maximized)
                navigation.host.showNormal();
            else
                navigation.host.showMaximized();
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 22 + navigation.captionLeft
        anchors.rightMargin: 22 + navigation.captionRight
        spacing: navigation.host.width > 1250 ? 16 : 10

        Image {
            source: "../assets/branding/church-deck-" + (LibraryStyle.dark ? "light" : "dark")
                    + ".svg"
            sourceSize.width: 64
            sourceSize.height: 64
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
        }

        Text {
            visible: navigation.host.width >= 1240
            text: "Selah"
            color: LibraryStyle.tone("#F2F2F2")
            font.pixelSize: 15
            font.weight: Font.DemiBold
            font.letterSpacing: -0.4
        }

        Rectangle {
            implicitWidth: 1
            implicitHeight: 24
            color: LibraryStyle.tone("#353535")
            Layout.leftMargin: 8
            Layout.rightMargin: 8
        }

        Rectangle {
            implicitWidth: workspaceTabs.implicitWidth + 8
            implicitHeight: 40
            radius: 8
            color: LibraryStyle.tone("#141414")
            border.color: LibraryStyle.divider

            Row {
                id: workspaceTabs

                x: 4
                y: 4
                spacing: 2

                Repeater {
                    model: [
                        {
                            text: "Programs",
                            value: "programs",
                            icon: "list-ordered"
                        },
                        {
                            text: "Libraries",
                            value: "libraries",
                            icon: "layers"
                        },
                        {
                            text: "Design",
                            value: "design",
                            icon: "pen-tool"
                        },
                        {
                            text: "Present",
                            value: "present",
                            icon: "monitor-play"
                        }
                    ]

                    LibraryButton {
                        required property var modelData

                        objectName: "workspaceNav_" + modelData.value
                        text: modelData.text
                        iconName: modelData.icon
                        selected: navigation.host.activeWorkspace === modelData.value
                        quiet: !selected
                        implicitHeight: 32
                        labelFontSize: 12
                        hint: modelData.text + " workspace"

                        onClicked: navigation.host.switchWorkspace(modelData.value)
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Rectangle {
            visible: navigation.host.outputs.liveCount > 0
            implicitWidth: liveLabel.implicitWidth + 28
            implicitHeight: 28
            radius: 14
            color: navigation.host.outputs.audienceBlack ? "#392D21" : "#25312A"

            Text {
                id: liveLabel

                anchors.centerIn: parent
                text: navigation.host.outputs.audienceBlack ? "AUDIENCE BLACKOUT" : "LIVE · " + (
                                                                  navigation.host.outputs.liveIndex
                                                                  + 1) + " / "
                                                              + navigation.host.outputs.liveCount
                color: navigation.host.outputs.audienceBlack ? "#E7BE91" : "#B5D8C1"
                font.pixelSize: 10
                font.letterSpacing: 0.7
            }
        }

        UserGuideDialog {
            id: userGuide
        }

        LibraryButton {
            objectName: "userGuideButton"
            iconName: "book-open"
            implicitWidth: LibraryStyle.controlHeight
            implicitHeight: LibraryStyle.controlHeight
            quiet: true
            hint: "Getting started and user guides"
            Accessible.name: "Open user guide"

            onClicked: userGuide.open()
        }

        FeedbackButton {
        }

        LibraryButton {
            id: appearanceButton

            readonly property string currentIcon: LibraryStyle.dark ? "sun" : "moon"
            readonly property string sunSvg:
                '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="#cccccc" stroke-width="1.5" stroke-linecap="round"><circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M2 12h2m16 0h2M4.93 4.93l1.42 1.42m11.3 11.3 1.42 1.42M4.93 19.07l1.42-1.42m11.3-11.3 1.42-1.42"/></svg>'
            readonly property string moonSvg:
                '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="#cccccc" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M20.5 13A8.5 8.5 0 0 1 11 3.5 8.5 8.5 0 1 0 20.5 13Z"/></svg>'

            function setMode(mode) {
                LibraryStyle.setAppearance(mode);
            }

            objectName: "appearancePicker"
            implicitWidth: LibraryStyle.controlHeight
            implicitHeight: LibraryStyle.controlHeight
            quiet: true
            hint: (LibraryStyle.dark ? "Switch to light appearance" : "Switch to dark appearance")
                  + " · Right-click for System"

            contentItem: Item {
                StudioIcon {
                    anchors.centerIn: parent
                    width: 18
                    height: 18
                    source: "data:image/svg+xml," + encodeURIComponent(appearanceButton.currentIcon
                                                                       === "sun"
                                                                       ? appearanceButton.sunSvg :
                                                                         appearanceButton.moonSvg)
                    tone: LibraryStyle.text
                }
            }

            onClicked: setMode(LibraryStyle.dark ? "light" : "dark")
            onPressAndHold: appearanceMenu.open()

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton

                onClicked: appearanceMenu.open()
            }

            Popup {
                id: appearanceMenu

                y: appearanceButton.height + 4
                x: -140 + appearanceButton.width
                padding: 4

                background: Rectangle {
                    color: LibraryStyle.panel
                    border.color: LibraryStyle.border
                    radius: 6
                }
                contentItem: Column {
                    spacing: 2

                    Repeater {
                        model: [
                            {
                                label: "Light",
                                value: "light"
                            },
                            {
                                label: "Dark",
                                value: "dark"
                            },
                            {
                                label: "Use system setting",
                                value: "system"
                            }
                        ]

                        LibraryButton {
                            required property var modelData

                            width: 170
                            text: modelData.label
                            selected: LibraryStyle.appearance === modelData.value
                            quiet: !selected

                            onClicked: {
                                appearanceButton.setMode(modelData.value);
                                appearanceMenu.close();
                            }
                        }
                    }
                }
            }
        }

        StudioButton {
            text: "Outputs"

            onClicked: navigation.host.showOutputs()
        }

        LibraryButton {
            objectName: "remoteToolbarButton"
            iconName: "smartphone"
            implicitWidth: LibraryStyle.controlHeight
            implicitHeight: LibraryStyle.controlHeight
            quiet: true
            hint: navigation.host.remote.running ? "Remote control is running" :
                                                   "Open remote control"
            checked: navigation.host.remote.running

            onClicked: navigation.host.showRemote()
        }
    }
}
