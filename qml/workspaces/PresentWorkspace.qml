pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Item {
    id: present

    required property var host
    readonly property bool quickAddOpen: quickDialog.opened || scriptureDrawer.opened
                                         || cueBar.dialogOpen
    property var controller: host.outputs
    readonly property bool controlHasFocus: {
        let item = host.activeFocusItem;
        return !!item && (item.pressed !== undefined || item.cursorPosition !== undefined
                          || item.currentIndex !== undefined);
    }
    property bool canNavigate: visible && !host.settingsOpen && !controlHasFocus
                               && controller.liveCount > 0

    function openOutputControls(section = "displays") {
        outputControls.section = section;
        Qt.callLater(() => {
            const viewport = presentScroll.contentItem as Flickable;
            if (!viewport)
                return;
            viewport.contentY = Math.max(0, Math.min(outputControls.y,
                                                     presentContent.implicitHeight
                                                     - presentScroll.availableHeight));
        });
    }

    function navigate(delta) {
        controller.advance(delta);
        forceActiveFocus();
    }

    objectName: "presentWorkspace"

    onVisibleChanged: if (!visible)
                          scriptureDrawer.close()

    QuickAddDialog {
        id: quickDialog

        host: present.host
        parent: Overlay.overlay
    }

    Drawer {
        id: scriptureDrawer

        objectName: "scriptureSessionPanel"
        parent: Overlay.overlay
        edge: Qt.RightEdge
        width: Math.min(1120, parent.width * 0.83)
        height: parent.height
        modal: false
        dim: false
        closePolicy: Popup.CloseOnEscape

        onOpened: {
            scriptureBrowser.restorePreferences({
                                                    bible: scripturePreferences.bible,
                                                    second: scripturePreferences.second,
                                                    layout: scripturePreferences.layout
                                                });
            scriptureBrowser.open();
        }
        onClosed: {
            const preferences = scriptureBrowser.sessionPreferences;
            scripturePreferences.bible = preferences.bible;
            scripturePreferences.second = preferences.second;
            scripturePreferences.layout = preferences.layout;
            scriptureBrowser.close();
        }

        Settings {
            id: scripturePreferences

            property string bible: ""
            property string second: ""
            property string layout: ""

            category: "ScriptureSession"
        }

        BibleLibraryPanel {
            id: scriptureBrowser

            objectName: "scriptureSessionBible"
            parent: scriptureDrawer.contentItem
            workspaceMode: true
            liveMode: true
            library: present.host.content
            templates: present.host.templates
            document: present.host.doc
            sessionController: present.host.quickAdd
            outputs: present.controller

            onSessionClosed: scriptureDrawer.close()
        }
    }

    Shortcut {
        sequence: Qt.platform.os === "osx" ? "Meta+K" : "Ctrl+K"
        enabled: present.visible && !present.host.settingsOpen

        onActivated: quickDialog.open()
    }

    Shortcut {
        sequence: "Right"
        enabled: present.canNavigate

        onActivated: present.navigate(1)
    }

    Shortcut {
        sequence: "Space"
        enabled: present.canNavigate

        onActivated: present.navigate(1)
    }

    Shortcut {
        sequence: "Left"
        enabled: present.canNavigate

        onActivated: present.navigate(-1)
    }

    Shortcut {
        sequence: "Home"
        enabled: present.canNavigate

        onActivated: present.navigate(-present.controller.liveIndex)
    }

    Shortcut {
        sequence: "End"
        enabled: present.canNavigate

        onActivated: present.navigate(present.controller.liveCount - 1
                                      - present.controller.liveIndex)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true

                Text {
                    text: "Present"
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Medium
                    color: LibraryStyle.text
                }

                Text {
                    text: present.controller.liveCount ? present.controller.liveProgramTitle
                                                         + " · " + present.controller.liveCount
                                                         + " slides" :
                                                         "Choose a program or show your standby media."
                    font.pixelSize: 11
                    color: LibraryStyle.muted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Text {
                text: present.controller.audienceEnabled ? "● Audience on" : "○ Audience off"
                color: present.controller.audienceEnabled ? (LibraryStyle.dark ? "#A8CFB6" :
                                                                                 "#306044") :
                                                            LibraryStyle.muted
                font.pixelSize: 11
            }

            StudioButton {
                objectName: "scriptureSessionOpenButton"
                text: "Scripture"
                iconName: "book-open"
                selected: scriptureDrawer.opened

                onClicked: scriptureDrawer.opened ? scriptureDrawer.close() : scriptureDrawer.open()
            }

            StudioButton {
                objectName: "quickAddOpenButton"
                text: "Quick Add"
                iconName: "plus"
                accented: true
                hint: Qt.platform.os === "osx" ? "Quick Add (⌘K)" : "Quick Add (Ctrl+K)"

                onClicked: quickDialog.open()
            }

            StudioButton {
                objectName: "presentAudiencePreviewButton"
                text: present.controller.audiencePreviewOpen ? "Close audience preview" :
                                                               "Audience preview"
                iconName: present.controller.audiencePreviewOpen ? "x" : "monitor-play"
                selected: present.controller.audiencePreviewOpen
                enabled: present.controller.audiencePreviewOpen
                         || present.controller.hasAudienceContent

                onClicked: present.controller.audiencePreviewOpen ? present.controller.stop(
                                                                        "audience") :
                                                                    present.controller.preview(
                                                                        "audience")
            }
        }

        SquircleSurface {
            visible: present.host.recovery.available
            Layout.fillWidth: true
            implicitHeight: recoveryContent.implicitHeight + 24
            color: LibraryStyle.field
            border.color: LibraryStyle.border
            radius: 10

            RowLayout {
                id: recoveryContent

                anchors.fill: parent
                anchors.margins: 12

                ColumnLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "Recover interrupted session?"
                        color: LibraryStyle.text
                        font.weight: Font.Medium
                    }

                    Text {
                        text: present.host.recovery.summary
                              + "\nOutputs stay off until you start them."
                        color: LibraryStyle.muted
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        font.pixelSize: 12
                    }
                }

                LibraryButton {
                    text: "Discard"

                    onClicked: present.host.recovery.discard()
                }

                LibraryButton {
                    text: "Restore session"
                    accented: true

                    onClicked: present.host.recovery.recover()
                }
            }
        }

        Text {
            visible: present.host.recovery.status.length > 0
            text: present.host.recovery.status
            color: LibraryStyle.muted
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ServiceCueBar {
            id: cueBar

            Layout.fillWidth: true
            controller: present.host.serviceCues
        }

        PresentProgramPicker {
            host: present.host
        }

        RowLayout {
            id: body

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: 0
            spacing: 16

            PresentSlideList {
                controller: present.controller
                Layout.preferredWidth: present.width > 1250 ? 232 : 205
                Layout.fillHeight: true

                onChosen: index => present.navigate(index - present.controller.liveIndex)
                onPrepareRequested: present.host.showProgramEditor()
            }

            ScrollView {
                id: presentScroll

                objectName: "presentContentScroll"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    id: presentContent

                    width: presentScroll.availableWidth
                    spacing: 20

                    RowLayout {
                        objectName: "presentDisplayPreviews"
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 2
                            Layout.alignment: Qt.AlignTop
                            spacing: 16

                            LiveSlideCard {
                                controller: present.controller
                                Layout.fillWidth: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.alignment: Qt.AlignTop
                            spacing: 16

                            LiveSlideCard {
                                controller: present.controller
                                next: true
                                Layout.fillWidth: true
                            }

                            PresentImagePanel {
                                controller: present.controller
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: LibraryStyle.divider
                    }

                    OutputControls {
                        id: outputControls

                        Layout.fillWidth: true
                        controller: present.controller

                        onRemoteRequested: present.host.showRemote()
                        onBroadcastRequested: present.host.showNdi()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: present.controller.audienceBlack || present.controller.textCleared
                     || present.controller.defaultImageShowing
            text: present.controller.audienceBlack
                  ? "Audience is blacked out. Restore it when you’re ready." :
                    present.controller.defaultImageShowing
                    ? "Standby media is showing · Resume returns to the same program slide." :
                      "Audience text is cleared. Background media continues."
            color: LibraryStyle.dark ? "#D4B993" : "#7D572A"
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }

        PresentTransport {
            controller: present.controller
            presenting: present.host.presentationRunning
            Layout.fillWidth: true

            onStopRequested: present.host.stopPresenting()
            onNavigate: delta => present.navigate(delta)
            onOutputsRequested: present.host.showOutputs()
        }
    }
}
