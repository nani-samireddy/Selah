pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

// Qt places this bar in the system menu on macOS and inside the window elsewhere.
MenuBar {
    id: menus

    required property var host
    property var menuTextTarget: null
    readonly property var focusedText: host.activeFocusItem
                                       && typeof host.activeFocusItem.selectAll === "function"
                                       ? host.activeFocusItem : host.activeTextEditor
    readonly property var textTarget: menuTextTarget || focusedText
    readonly property bool design: host.activeWorkspace === "design"
    readonly property bool programs: host.activeWorkspace === "programs"
    readonly property var historyTarget: (textTarget === host.activeTextEditor && design ? host.doc :
                                                                                           textTarget)
                                         || (!host.settingsOpen ? (design ? host.doc : programs
                                                                            && host.programEditorOpen
                                                                            ? host.programs : null) :
                                                                  null)

    objectName: "applicationMenuBar"
    height: Qt.platform.os === "osx" ? 0 : implicitHeight
    clip: true

    ActionGroup {
        id: workspaceActions
    }

    ActionGroup {
        id: appearanceActions
    }

    Menu {
        title: qsTr("&File")

        Action {
            objectName: "menuNewProgram"
            text: qsTr("New program…")
            shortcut: StandardKey.New
            enabled: !menus.host.settingsOpen

            onTriggered: menus.host.newProgram()
        }

        Action {
            text: qsTr("New presentation…")
            enabled: !menus.host.settingsOpen

            onTriggered: menus.host.newPresentation()
        }

        Action {
            text: qsTr("Open presentation…")
            shortcut: StandardKey.Open
            enabled: !menus.host.settingsOpen

            onTriggered: menus.host.openPresentation()
        }

        MenuSeparator {
        }

        Action {
            objectName: "menuSave"
            text: menus.programs ? qsTr("Save program") : menus.host.templateMode ? qsTr(
                                                                                        "Save template…") :
                                                                                    qsTr("Save presentation…")
            shortcut: StandardKey.Save
            enabled: (menus.design || (menus.programs && menus.host.programEditorOpen)) &&
                     !menus.host.settingsOpen

            onTriggered: menus.host.saveActiveWorkspace()
        }

        Action {
            objectName: "menuImportProgram"
            text: qsTr("Import program…")
            enabled: !menus.host.settingsOpen

            onTriggered: menus.host.importProgram()
        }

        Action {
            objectName: "menuExportProgram"
            text: qsTr("Export program…")
            enabled: !!menus.host.programs.id && (!menus.programs || menus.host.programEditorOpen)
                     && !menus.host.settingsOpen

            onTriggered: menus.host.exportProgram()
        }

        Action {
            objectName: "menuExportPortableProgram"
            text: qsTr("Export portable program…")
            enabled: !!menus.host.programs.id && !menus.host.settingsOpen

            onTriggered: menus.host.exportPortableProgram()
        }

        Action {
            text: qsTr("Backup & restore…")
            enabled: !menus.host.settingsOpen && !menus.host.programs.bundleBusy

            onTriggered: menus.host.showBackup()
        }

        Action {
            text: qsTr("Export presentation…")
            shortcut: Qt.platform.os === "osx" ? "Meta+Shift+S" : "Ctrl+Shift+S"
            enabled: menus.design && !menus.host.settingsOpen

            onTriggered: menus.host.exportPresentation()
        }

        MenuSeparator {
        }

        Action {
            text: Qt.platform.os === "osx" ? qsTr("Quit Selah") : qsTr("Exit")
            shortcut: StandardKey.Quit

            onTriggered: {
                if (menus.host.close())
                    Qt.quit();
            }
        }
    }

    Menu {
        title: qsTr("&Edit")

        onAboutToShow: menus.menuTextTarget = menus.focusedText
        onClosed: Qt.callLater(() => menus.menuTextTarget = null)

        Action {
            objectName: "menuUndo"
            text: qsTr("Undo")
            shortcut: StandardKey.Undo
            enabled: !!menus.historyTarget && !menus.historyTarget.readOnly && !
                     !menus.historyTarget.canUndo

            onTriggered: {
                if (menus.historyTarget === menus.host.doc)
                    menus.host.undoEditor();
                else
                    menus.historyTarget.undo();
            }
        }

        Action {
            objectName: "menuRedo"
            text: qsTr("Redo")
            shortcut: StandardKey.Redo
            enabled: !!menus.historyTarget && !menus.historyTarget.readOnly && !
                     !menus.historyTarget.canRedo

            onTriggered: {
                if (menus.historyTarget === menus.host.doc)
                    menus.host.redoEditor();
                else
                    menus.historyTarget.redo();
            }
        }

        MenuSeparator {
        }

        Action {
            text: qsTr("Cut")
            shortcut: StandardKey.Cut
            enabled: !!menus.textTarget && !menus.textTarget.readOnly
                     && menus.textTarget.selectedText.length > 0

            onTriggered: menus.textTarget.cut()
        }

        Action {
            text: qsTr("Copy")
            shortcut: StandardKey.Copy
            enabled: !!menus.textTarget && menus.textTarget.selectedText.length > 0

            onTriggered: menus.textTarget.copy()
        }

        Action {
            text: qsTr("Paste")
            shortcut: StandardKey.Paste
            enabled: !!menus.textTarget && !menus.textTarget.readOnly && !!menus.textTarget.canPaste

            onTriggered: menus.textTarget.paste()
        }

        Action {
            text: qsTr("Select all")
            shortcut: StandardKey.SelectAll
            enabled: !!menus.textTarget

            onTriggered: menus.textTarget.selectAll()
        }

        MenuSeparator {
        }

        Action {
            text: qsTr("Duplicate layer")
            shortcut: Qt.platform.os === "osx" ? "Meta+D" : "Ctrl+D"
            enabled: menus.design && menus.host.editorShortcutsAllowed && !menus.textTarget
                     && menus.host.doc.selected >= 0

            onTriggered: menus.host.doc.duplicateSelected()
        }

        Action {
            text: qsTr("Delete layer")
            enabled: menus.design && menus.host.editorShortcutsAllowed && !menus.textTarget
                     && menus.host.doc.selected >= 0

            onTriggered: menus.host.doc.deleteSelected()
        }
    }

    Menu {
        title: qsTr("&View")

        Action {
            text: qsTr("Programs")
            enabled: !menus.host.settingsOpen
            ActionGroup.group: workspaceActions
            checkable: true
            checked: menus.programs

            onTriggered: menus.host.switchWorkspace("programs")
        }

        Action {
            objectName: "menuLibraries"
            text: qsTr("Libraries")
            enabled: !menus.host.settingsOpen
            ActionGroup.group: workspaceActions
            checkable: true
            checked: menus.host.activeWorkspace === "libraries"

            onTriggered: menus.host.switchWorkspace("libraries")
        }

        Action {
            text: qsTr("Design")
            enabled: !menus.host.settingsOpen
            ActionGroup.group: workspaceActions
            checkable: true
            checked: menus.design

            onTriggered: menus.host.switchWorkspace("design")
        }

        Action {
            text: qsTr("Present")
            enabled: !menus.host.settingsOpen
            ActionGroup.group: workspaceActions
            checkable: true
            checked: menus.host.activeWorkspace === "present"

            onTriggered: menus.host.switchWorkspace("present")
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Appearance")

            Action {
                text: qsTr("Light")
                ActionGroup.group: appearanceActions
                checkable: true
                checked: LibraryStyle.appearance === "light"

                onTriggered: LibraryStyle.setAppearance("light")
            }

            Action {
                text: qsTr("Dark")
                ActionGroup.group: appearanceActions
                checkable: true
                checked: LibraryStyle.appearance === "dark"

                onTriggered: LibraryStyle.setAppearance("dark")
            }

            Action {
                text: qsTr("Follow system")
                ActionGroup.group: appearanceActions
                checkable: true
                checked: LibraryStyle.appearance === "system"

                onTriggered: LibraryStyle.setAppearance("system")
            }
        }

        Action {
            text: menus.host.visibility === Window.FullScreen ? qsTr("Exit full screen") : qsTr(
                                                                    "Enter full screen")
            shortcut: Qt.platform.os === "osx" ? "Meta+Ctrl+F" : "F11"

            onTriggered: menus.host.visibility === Window.FullScreen ? menus.host.showNormal() :
                                                                       menus.host.showFullScreen()
        }
    }

    Menu {
        title: qsTr("&Presentation")

        Action {
            text: qsTr("Output settings…")

            onTriggered: menus.host.showOutputs()
        }

        Action {
            text: qsTr("Remote control…")

            onTriggered: menus.host.showRemote()
        }

        Action {
            text: qsTr("NDI output…")

            onTriggered: menus.host.showNdi()
        }

        MenuSeparator {
        }

        Action {
            text: qsTr("Previous slide")
            enabled: menus.host.outputs.liveIndex > 0

            onTriggered: menus.host.outputs.advance(-1)
        }

        Action {
            text: qsTr("Next slide")
            enabled: menus.host.outputs.liveIndex >= 0 && menus.host.outputs.liveIndex
                     < menus.host.outputs.liveCount - 1

            onTriggered: menus.host.outputs.advance(1)
        }

        Action {
            text: qsTr("Audience blackout")
            checkable: true
            checked: menus.host.outputs.audienceBlack
            enabled: menus.host.outputs.hasAudienceContent

            onTriggered: menus.host.outputs.blackout("audience", !menus.host.outputs.audienceBlack)
        }

        Action {
            objectName: "menuStopPresenting"
            text: qsTr("Stop presenting")
            enabled: menus.host.presentationRunning

            onTriggered: menus.host.stopPresenting()
        }

        Action {
            text: qsTr("Show standby media")
            checkable: true
            checked: menus.host.outputs.defaultImageShowing
            enabled: menus.host.outputs.defaultImageAvailable

            onTriggered: menus.host.outputs.showDefaultImage(
                             !menus.host.outputs.defaultImageShowing)
        }
    }

    Menu {
        title: qsTr("&Help")

        Action {
            objectName: "menuCheckUpdates"
            text: qsTr("Check for updates…")

            onTriggered: menus.host.showUpdates()
        }

        Action {
            text: qsTr("Luma…")

            onTriggered: menus.host.showAiAssistant()
        }

        Action {
            objectName: "menuUserGuide"
            text: qsTr("Selah user guide")
            shortcut: "F1"

            onTriggered: menus.host.showUserGuide()
        }
    }
}
