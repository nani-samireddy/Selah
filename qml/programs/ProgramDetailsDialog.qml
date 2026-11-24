pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var store
    property bool creating: false
    property bool attempted: false

    signal saved

    objectName: creating ? "newProgramDialog" : "editProgramDialog"
    title: creating ? "New program" : "Program details"
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 400
    padding: 20

    background: Rectangle {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 10
    }
    contentItem: ColumnLayout {
        spacing: 12

        LibraryCaption {
            text: "Program name"
        }

        LibraryField {
            id: nameField

            objectName: dialog.creating ? "newProgramTitle" : "editProgramTitle"
            Layout.fillWidth: true
            placeholderText: "Sunday Worship"
            Accessible.name: "Program name"
        }

        LibraryCaption {
            text: "Service date (optional)"
        }

        LibraryField {
            id: dateField

            objectName: "programDetailsDate"
            Layout.fillWidth: true
            placeholderText: "YYYY-MM-DD"
            Accessible.name: "Service date"
        }

        Text {
            visible: dialog.attempted
            Layout.fillWidth: true
            text: dialog.store.status
            color: LibraryStyle.muted
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.topMargin: 8

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Cancel"

                onClicked: dialog.close()
            }

            LibraryButton {
                objectName: dialog.creating ? "createProgramConfirm" : "saveProgramDetailsButton"
                text: dialog.creating ? "Create program" : "Save changes"
                accented: true

                onClicked: {
                    dialog.attempted = true;
                    let ok = false;
                    if (dialog.creating)
                        ok = dialog.store.createProgram(nameField.text, dateField.text);
                    else {
                        dialog.store.updateDetails(nameField.text, dateField.text);
                        ok = dialog.store.title === nameField.text.trim() && dialog.store.date
                                === dateField.text && dialog.store.save();
                    }
                    if (ok) {
                        dialog.close();
                        dialog.saved();
                    }
                }
            }
        }
    }

    onOpened: {
        attempted = false;
        nameField.text = creating ? "Sunday Worship" : store.title;
        dateField.text = creating ? Qt.formatDate(new Date(), "yyyy-MM-dd") : store.date;
        nameField.forceActiveFocus();
        nameField.selectAll();
    }
}
