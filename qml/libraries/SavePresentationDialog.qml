import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property var host
    property bool createBlank: false

    function save(asCopy = false) {
        if (host.activeTextEditor)
            host.activeTextEditor.finish();
        const slides = createBlank ? [
                                         {
                                             name: "Slide 1",
                                             background: "#171717",
                                             ambientOpacity: 0,
                                             elements: []
                                         }
                                     ] : host.doc.slides;
        const id = host.presentations.save(nameField.text, slides, createBlank || asCopy ? "" :
                                                                                           host.editingPresentationId);
        if (!id)
            return;
        if (createBlank)
            host.editCustomPresentation(host.presentations.presentation(id));
        else {
            host.editingPresentationId = id;
            host.customPresentationTitle = nameField.text.trim();
            host.doc.saveProject();
            host.notify("Presentation saved to Presentations");
        }
        close();
    }

    objectName: "savePresentationDialog"
    title: createBlank ? "New custom presentation" : "Save custom presentation"
    modal: true
    anchors.centerIn: parent
    width: 400
    padding: 20

    background: SquircleSurface {
        radius: 10
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
    }
    contentItem: ColumnLayout {
        spacing: 12

        LibraryField {
            id: nameField

            objectName: "customPresentationName"
            Layout.fillWidth: true
            placeholderText: "Presentation name"

            onAccepted: dialog.save()
        }

        Text {
            Layout.fillWidth: true
            text: dialog.host.presentations.status
            wrapMode: Text.Wrap
            color: LibraryStyle.muted
            font.pixelSize: 12
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Cancel"

                onClicked: dialog.close()
            }

            LibraryButton {
                text: "Save a copy"
                visible: !dialog.createBlank && dialog.host.editingPresentationId.length > 0

                onClicked: dialog.save(true)
            }

            LibraryButton {
                text: dialog.createBlank ? "Create & edit" : "Save"
                accented: true
                enabled: nameField.text.trim().length > 0

                onClicked: dialog.save()
            }
        }
    }

    onOpened: {
        nameField.text = createBlank ? "Untitled presentation" : host.customPresentationTitle
                                       || "Untitled presentation";
        nameField.forceActiveFocus();
        nameField.selectAll();
    }
}
