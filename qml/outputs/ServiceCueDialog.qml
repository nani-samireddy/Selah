pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: dialog

    required property var controller
    property int editing: -1
    property int browsing: -1

    function edit(index) {
        editing = index;
        const cue = index < 0 ? {
                                    name: "",
                                    actions: []
                                } : controller.cues[index];
        nameField.text = cue.name;
        actions.clear();
        for (const action of cue.actions)
            actions.append({
                               payload: action
                           });
    }

    function gather() {
        let result = [];
        for (let i = 0; i < actions.count; i++)
            result.push(Object.assign({}, actions.get(i).payload));
        return {
            version: 2,
            name: nameField.text.trim(),
            actions: result
        };
    }

    function change(index, key, value) {
        let action = Object.assign({}, actions.get(index).payload);
        action[key] = value;
        actions.setProperty(index, "payload", action);
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(760, parent.width - 40)
    height: Math.min(800, parent.height - 40)
    modal: true
    padding: 24

    background: SquircleSurface {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 16
    }
    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            Text {
                text: "Service cues"
                color: LibraryStyle.text
                font.pixelSize: 22
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Close"

                onClicked: dialog.close()
            }
        }

        Text {
            text: "Actions run from top to bottom. Test checks the setup without changing live content or sending commands."
            color: LibraryStyle.muted
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        RowLayout {
            LibraryCombo {
                Layout.fillWidth: true
                model: dialog.controller.cues
                textRole: "name"
                currentIndex: dialog.editing
                enabled: !dialog.controller.running
                Accessible.name: "Cue to edit"

                onActivated: dialog.edit(currentIndex)
            }

            LibraryButton {
                text: "New cue"
                enabled: !dialog.controller.running

                onClicked: dialog.edit(-1)
            }
        }

        LibraryField {
            id: nameField

            Layout.fillWidth: true
            placeholderText: "Cue name"
            maximumLength: 60
            Accessible.name: "Cue name"
            enabled: !dialog.controller.running
        }

        ScrollView {
            id: actionScroll

            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: actionScroll.availableWidth
                spacing: 10

                Text {
                    visible: actions.count === 0
                    text: "Add your first action below."
                    color: LibraryStyle.muted
                    Layout.topMargin: 16
                }

                Repeater {
                    model: actions

                    ServiceCueActionCard {
                        id: actionCard

                        Layout.fillWidth: true
                        controller: dialog.controller
                        actionCount: actions.count
                        enabled: !dialog.controller.running

                        onEdited: (key, value) => dialog.change(actionCard.index, key, value)
                        onMoveRequested: delta => actions.move(actionCard.index, actionCard.index
                                                               + delta, 1)
                        onRemoveRequested: actions.remove(actionCard.index)
                        onBrowseRequested: kind => {
                            dialog.browsing = actionCard.index;
                            assetFile.nameFilters = kind === "image"
                                    ? ["Images (*.png *.jpg *.jpeg *.webp *.gif *.svg)"] :
                                      ["Audio (*.wav *.mp3 *.ogg *.m4a *.aac *.flac *.aiff *.aif *.opus)"];
                            assetFile.open();
                        }
                    }
                }
            }
        }

        RowLayout {
            LibraryCombo {
                id: actionType

                Layout.fillWidth: true
                model: dialog.controller.actionTypes
                textRole: "label"
                valueRole: "type"
                Accessible.name: "Action type to add"
                enabled: !dialog.controller.running
            }

            LibraryButton {
                text: "Add action"
                iconName: "plus"
                enabled: !dialog.controller.running && actions.count < 32

                onClicked: actions.append({
                                              payload: dialog.controller.newAction(
                                                           actionType.currentValue)
                                          })
            }
        }

        Text {
            text: dialog.controller.status
            color: LibraryStyle.muted
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            font.pixelSize: 12
        }

        RowLayout {
            LibraryButton {
                text: "Delete cue"
                enabled: dialog.editing >= 0 && !dialog.controller.running

                onClicked: if (dialog.controller.removeCue(dialog.editing))
                               dialog.edit(-1)
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Test"
                enabled: actions.count > 0 && !dialog.controller.running

                onClicked: dialog.controller.testCue(dialog.gather())
            }

            LibraryButton {
                text: dialog.controller.running ? "Cancel run" : "Run cue"
                enabled: actions.count > 0

                onClicked: dialog.controller.running ? dialog.controller.cancel() :
                                                       dialog.controller.runCue(dialog.gather())
            }

            LibraryButton {
                text: "Save cue"
                accented: true
                enabled: nameField.text.trim().length > 0 && actions.count > 0 &&
                         !dialog.controller.running

                onClicked: {
                    const target = dialog.editing < 0 ? dialog.controller.cues.length :
                                                        dialog.editing;


                    if (dialog.controller.saveCue(dialog.editing, dialog.gather()))
                        dialog.edit(target);
                }
            }
        }
    }

    onOpened: edit(controller.cues.length ? 0 : -1)

    ListModel {
        id: actions

        dynamicRoles: true
    }

    FileDialog {
        id: assetFile

        title: "Choose cue asset"

        onAccepted: if (dialog.browsing >= 0 && dialog.browsing < actions.count)
                        dialog.change(dialog.browsing, "source", selectedFile.toString())
    }
}
