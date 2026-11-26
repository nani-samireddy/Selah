pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog

    required property var installer
    required property var controller
    required property var content
    property bool setupOpen: false
    property bool localFilesOpen: false
    property string searchScope: "songs"
    property url chosenRuntime
    property url chosenModel
    readonly property var rows: controller.results.reduce((all, group) => all.concat(group.items.map(item
                                                                                                     => ({
                                                                                                         tool: group.tool,
                                                                                                         item: item
                                                                                                     }))), [])

    function bibleName(id) {
        const translation = content.translations.find(item => item.id === id);
        return translation ? translation.abbreviation || translation.title : "No Bible selected";
    }

    objectName: "aiAssistantDialog"
    title: "Luma"
    modal: true
    parent: Overlay.overlay
    width: Math.min(860, parent.width - 48)
    height: Math.min(720, parent.height - 48)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 20
    standardButtons: Dialog.Close

    background: SquircleSurface {
        radius: 12
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
    }
    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "Find content with Luma"
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 21
                    color: LibraryStyle.text
                }

                LibraryCaption {
                    text: "Runs on this computer · Review suggestions before searching your library"
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            LibraryButton {
                objectName: "aiEnabledToggle"
                text: dialog.controller.enabled ? "Luma on" : "Enable Luma"
                checkable: true
                checked: dialog.controller.enabled
                enabled: !dialog.controller.busy && !dialog.installer.busy

                onClicked: {
                    if (dialog.controller.enabled)
                        dialog.controller.setEnabled(false);
                    else if (dialog.controller.configured)
                        dialog.controller.setEnabled(true);
                    else
                        dialog.controller.useManagedModel();
                }
            }

            LibraryButton {
                iconName: "settings-2"
                implicitWidth: 30
                hint: "Luma setup · local AI model"

                onClicked: dialog.setupOpen = !dialog.setupOpen
            }
        }

        ColumnLayout {
            visible: dialog.setupOpen || !dialog.controller.configured
            Layout.fillWidth: true
            spacing: 8

            LibraryCaption {
                text: dialog.controller.configured
                      ? "Local model ready. Download again to verify or repair it, or choose local files." :
                        "Download the optional local model, or choose compatible runtime and model files."
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            LibraryCaption {
                text: dialog.installer.status
                Layout.fillWidth: true
                wrapMode: Text.Wrap
            }

            ProgressBar {
                visible: dialog.installer.busy
                Layout.fillWidth: true
                indeterminate: dialog.installer.total <= 0
                from: 0
                to: Math.max(1, dialog.installer.total)
                value: dialog.installer.received
            }

            RowLayout {
                Layout.fillWidth: true

                LibraryButton {
                    objectName: "downloadLumaButton"
                    text: dialog.installer.busy ? "Cancel download" : "Download Luma"
                    accented: !dialog.controller.configured
                    enabled: !dialog.controller.busy && dialog.installer.supported

                    onClicked: dialog.installer.busy ? dialog.installer.cancel() :
                                                       dialog.installer.install()
                }

                LibraryCaption {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: "Downloads from Cactus Compute on Hugging Face. Files are verified before use. Luma stays off until you enable it."
                }
            }

            LibraryCaption {
                visible: dialog.installer.busy && dialog.installer.total > 0
                text: (dialog.installer.received / 1048576).toFixed(1) + " / " + (
                          dialog.installer.total / 1048576).toFixed(1) + " MB"
            }

            LibraryButton {
                text: dialog.localFilesOpen ? "Hide local file setup" : "Use local files…"
                quiet: true
                enabled: !dialog.installer.busy && !dialog.controller.busy

                onClicked: dialog.localFilesOpen = !dialog.localFilesOpen
            }

            RowLayout {
                visible: dialog.localFilesOpen
                Layout.fillWidth: true

                LibraryButton {
                    text: "Choose runtime…"
                    enabled: !dialog.controller.busy && !dialog.installer.busy

                    onClicked: runtimePicker.open()
                }

                LibraryCaption {
                    text: dialog.chosenRuntime.toString() || dialog.controller.runtimePath
                          || "No runtime selected"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                visible: dialog.localFilesOpen
                Layout.fillWidth: true

                LibraryButton {
                    text: "Choose model…"
                    enabled: !dialog.controller.busy && !dialog.installer.busy

                    onClicked: modelPicker.open()
                }

                LibraryCaption {
                    text: dialog.chosenModel.toString() || dialog.controller.modelPath
                          || "No model selected"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                LibraryButton {
                    text: "Use files"
                    enabled: !dialog.controller.busy && !dialog.installer.busy
                             && dialog.chosenRuntime.toString().length > 0
                             && dialog.chosenModel.toString().length > 0

                    onClicked: {
                        if (dialog.controller.configure(dialog.chosenRuntime, dialog.chosenModel)) {
                            dialog.controller.setEnabled(true);
                            dialog.setupOpen = false;
                        }
                    }
                }
            }
        }

        LibraryTabs {
            objectName: "aiSearchScope"
            options: [
                {
                    text: "Songs",
                    value: "songs"
                },
                {
                    text: "Bible",
                    value: "scripture"
                }
            ]
            currentValue: dialog.searchScope
            enabled: !dialog.controller.busy && !dialog.installer.busy

            onChosen: value => {
                dialog.searchScope = value;
                dialog.controller.cancel();
            }
        }

        RowLayout {
            visible: dialog.searchScope === "scripture"
            Layout.fillWidth: true

            LibraryCaption {
                text: "Search Bible"
            }

            LibraryCombo {
                id: bible

                objectName: "aiBibleSelector"
                Layout.fillWidth: true
                model: dialog.content.translations
                textRole: "title"
                displayText: count ? currentText : "No Bibles installed"
                enabled: !dialog.controller.busy && !dialog.installer.busy && count > 0
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryField {
                id: prompt

                objectName: "aiPrompt"
                Layout.fillWidth: true
                placeholderText: dialog.searchScope === "songs" ? "Find songs about grace" :
                                                                  "Find Genesis 1:1"
                enabled: dialog.controller.enabled && !dialog.controller.busy &&
                         !dialog.installer.busy
                maximumLength: 2000

                onAccepted: dialog.controller.propose(text, bible.currentIndex >= 0
                                                      ? bible.model[bible.currentIndex].id : "",
                                                      dialog.searchScope)
            }

            LibraryButton {
                objectName: "aiSuggestButton"
                text: dialog.controller.busy ? "Thinking…" : "Suggest searches"
                accented: true
                enabled: dialog.controller.enabled && !dialog.controller.busy &&
                         !dialog.installer.busy && prompt.text.trim().length > 0

                onClicked: dialog.controller.propose(prompt.text, bible.currentIndex >= 0
                                                     ? bible.model[bible.currentIndex].id : "",
                                                     dialog.searchScope)
            }

            LibraryButton {
                visible: dialog.controller.busy
                text: "Cancel"

                onClicked: dialog.controller.cancel()
            }
        }

        LibraryCaption {
            objectName: "aiStatus"
            text: dialog.controller.status
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ColumnLayout {
            visible: dialog.controller.proposals.length > 0
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: dialog.controller.proposals

                delegate: SquircleSurface {
                    id: proposal

                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: proposalText.implicitHeight + 20
                    color: LibraryStyle.field
                    radius: 7
                    border.color: LibraryStyle.border

                    Text {
                        id: proposalText

                        anchors.fill: parent
                        anchors.margins: 10
                        textFormat: Text.PlainText
                        text: (proposal.modelData.name === "search_songs" ? "Songs" :
                                                                            dialog.bibleName(
                                                                                proposal.modelData.arguments.translation_id))
                              + " · " + proposal.modelData.arguments.query + (
                                  proposal.modelData.originalQuery ? "\nSpelling suggestion for: "
                                                                     + proposal.modelData.originalQuery :
                                                                     "") + (proposal.modelData.confidence
                                                                            < 0.7 ? "\nCheck this suggestion carefully." :
                                                                                    "")
                        color: LibraryStyle.text
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                    }
                }
            }

            LibraryButton {
                objectName: "aiRunSearchesButton"
                text: "Search library"
                iconName: "search"
                accented: true

                onClicked: dialog.controller.runSearches()
            }
        }

        ListView {
            id: results

            objectName: "aiSearchResults"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: dialog.rows

            ScrollBar.vertical: ScrollBar {
            }
            delegate: SquircleSurface {
                id: result

                required property var modelData

                width: results.width
                height: resultText.implicitHeight + 24
                color: LibraryStyle.field
                radius: 7
                border.color: LibraryStyle.border

                ColumnLayout {
                    id: resultText

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 6

                    Text {
                        text: result.modelData.item.title || ((result.modelData.item.reference
                                                               || "Scripture") + " · " + (
                                                                  result.modelData.item.abbreviation
                                                                  || ""))
                        textFormat: Text.PlainText
                        color: LibraryStyle.text
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    LibraryCaption {
                        text: result.modelData.item.text || result.modelData.item.primaryLanguage
                              || ""
                        textFormat: Text.PlainText
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    onOpened: prompt.forceActiveFocus()
    onClosed: if (controller.busy)
                  controller.cancel()

    FileDialog {
        id: runtimePicker

        title: "Choose Cactus runtime"

        onAccepted: dialog.chosenRuntime = selectedFile
    }

    FileDialog {
        id: modelPicker

        title: "Choose Needle model"
        nameFilters: ["Cactus models (*.cact)"]

        onAccepted: dialog.chosenModel = selectedFile
    }
}
