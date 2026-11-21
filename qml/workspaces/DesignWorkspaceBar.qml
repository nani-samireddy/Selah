pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: bar

    required property var host

    implicitHeight: 54
    color: LibraryStyle.tone("#151515")

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: LibraryStyle.tone("#2C2C2C")
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 10

        Column {
            spacing: 3

            Text {
                text: bar.host.templateMode ? "Template design" : bar.host.programEditingItem
                                              ? "Program item design" :
                                                bar.host.customPresentationTitle
                                                || "Custom presentation"
                color: LibraryStyle.tone("#DEDEDE")
                font.pixelSize: 12
            }

            Text {
                text: bar.host.doc.importing ? "Importing media…" : bar.host.doc.status
                color: LibraryStyle.tone("#858585")
                font.pixelSize: 10
            }
        }

        Item {
            Layout.fillWidth: true
        }

        StudioButton {
            text: "Update program item"
            visible: bar.host.programEditingItem.length > 0
            enabled: bar.host.programEditingItem === bar.host.programs.selectedItem.id

            onClicked: {
                if (bar.host.programs.updateSelectedSlides(bar.host.doc.slides)) {
                    bar.host.notify("Program item updated");
                    bar.host.showPrograms();
                }
            }
        }

        StudioButton {
            visible: !bar.host.templateMode
            text: bar.host.editingPresentationId ? "Save presentation" : "Save to library"

            onClicked: bar.host.saveCustomPresentation()
        }

        StudioButton {
            text: "Open"
            quiet: true

            onClicked: bar.host.openPresentation()
        }

        StudioButton {
            text: "Export"
            iconName: "upload"

            onClicked: bar.host.exportPresentation()
        }

        StudioButton {
            text: "Preview"
            iconName: "play"

            onClicked: bar.host.preview()
        }

        StudioButton {
            text: bar.host.templateMode ? "Save template" : "Present slide"
            accented: true

            onClicked: {
                if (bar.host.templateMode)
                    bar.host.saveCurrentTemplate();
                else {
                    bar.host.outputs.takeLive(bar.host.doc.slides, bar.host.doc.currentSlide);
                    bar.host.switchWorkspace("present");
                }
            }
        }
    }
}
