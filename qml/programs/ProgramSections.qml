pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: controls

    required property var programView
    readonly property var store: programView.store
    readonly property string selectedSection: store.selectedItem.section || ""
    property Dialog sectionDialog: Dialog {
        id: editor

        property string originalName: ""
        property string problem: ""

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 360
        padding: 20
        modal: true
        title: originalName.length ? "Rename section" : "Add section"

        background: SquircleSurface {
            color: LibraryStyle.panel
            border.color: LibraryStyle.border
            radius: 10
        }
        contentItem: ColumnLayout {
            spacing: 12

            LibraryField {
                id: nameField

                objectName: "programSectionName"
                Layout.fillWidth: true
                placeholderText: "Worship, Sermon, Closing…"
                Accessible.name: "Section name"
                maximumLength: 100
            }

            Text {
                visible: editor.problem.length > 0
                text: editor.problem
                color: LibraryStyle.muted
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                Item {
                    Layout.fillWidth: true
                }

                LibraryButton {
                    text: "Cancel"

                    onClicked: editor.close()
                }

                LibraryButton {
                    text: editor.originalName.length ? "Save" : "Add section"
                    accented: true
                    enabled: nameField.text.trim().length > 0

                    onClicked: {
                        controls.programView.flushItem();
                        let ok = editor.originalName.length ? controls.store.renameSection(
                                                                  editor.originalName,
                                                                  nameField.text) :
                                                              controls.store.addSection(
                                                                  nameField.text);
                        if (ok)
                            editor.close();
                        else
                            editor.problem = "Choose a unique section name (maximum 40 sections).";
                    }
                }
            }
        }

        onOpened: {
            problem = "";
            nameField.text = originalName;
            nameField.forceActiveFocus();
            nameField.selectAll();
        }
    }

    LibraryCaption {
        text: "Section for selected item"
    }

    RowLayout {
        spacing: 4

        LibraryCombo {
            objectName: "programItemSection"
            Layout.fillWidth: true
            model: ["No section"].concat(controls.store.sections)
            currentIndex: controls.store.sections.indexOf(controls.selectedSection) + 1
            enabled: controls.store.selected >= 0
            Accessible.name: "Section for selected service item"

            onActivated: {
                controls.programView.flushItem();
                controls.store.setItemSection(currentIndex === 0 ? "" :
                                                                   controls.store.sections[currentIndex
                                                                                           - 1]);
            }
        }

        LibraryButton {
            iconName: "plus"
            hint: "Add a service section"
            enabled: controls.store.id.length > 0

            onClicked: {
                editor.originalName = "";
                editor.open();
            }
        }

        LibraryButton {
            iconName: "pen-tool"
            hint: "Rename selected section"
            enabled: controls.selectedSection.length > 0

            onClicked: {
                editor.originalName = controls.selectedSection;
                editor.open();
            }
        }

        LibraryButton {
            iconName: "trash-2"
            hint: "Remove section heading and keep its items"
            enabled: controls.selectedSection.length > 0

            onClicked: {
                controls.programView.flushItem();
                controls.store.removeSection(controls.selectedSection);
            }
        }
    }
}
