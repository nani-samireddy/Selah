pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: panel

    required property var library
    property var programTarget: null
    property string selectedId: ""
    property string filter: "all"
    property int previewIndex: 0
    property var result: ({
                              slides: [],
                              error: ""
                          })
    readonly property var records: library.layouts.filter(record => filter === "all" || filter
                                                                    === "scripture" && record.kind
                                                                    === "scripture" || filter
                                                                    === "bilingual" && record.kind
                                                                    === "bilingual" || filter
                                                                    === "song" && record.kind
                                                                    !== "scripture")
    readonly property var definition: library.layouts.find(record => record.id === selectedId) || (
                                          {})
    readonly property var previewSlide: result.slides[previewIndex] || ({})

    signal editRequested(var definition)

    function refreshPreview() {
        if (!opened)
            return;
        if (!records.some(record => record.id === selectedId))
            selectedId = records.length ? records[0].id : "";
        result = selectedId ? (programTarget ? programTarget.previewLayout(definition) :
                                               library.preview(selectedId)) : ({
                                                                                   slides: [],
                                                                                   error: ""
                                                                               });
        previewIndex = 0;
    }

    width: Math.min(1100, parent.width - 40)
    height: Math.min(740, parent.height - 40)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    focus: true
    padding: 22

    background: Rectangle {
        color: "#191919"
        border.color: "#3B3B3B"
        radius: 12
    }

    onOpened: {
        filter = programTarget ? programTarget.layoutKind : "all";
        refreshPreview();
    }
    onSelectedIdChanged: refreshPreview()
    onFilterChanged: refreshPreview()

    Connections {
        function onChanged() {
            panel.refreshPreview();
        }

        target: panel.library
    }

    FileDialog {
        id: importFile

        title: "Import template"
        nameFilters: ["Church Deck templates (*.churchtemplate)"]

        onAccepted: panel.library.importTemplate(selectedFile)
    }

    FileDialog {
        id: exportFile

        title: "Export template"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "churchtemplate"
        nameFilters: ["Church Deck templates (*.churchtemplate)"]

        onAccepted: panel.library.exportTemplate(panel.selectedId, selectedFile)
    }

    Dialog {
        id: createDialog

        title: "New template"
        modal: true
        anchors.centerIn: parent
        width: 360

        background: Rectangle {
            color: "#252525"
            border.color: "#444444"
            radius: 8
        }
        contentItem: ColumnLayout {
            LibraryField {
                id: newName

                placeholderText: "Template name"
                Layout.fillWidth: true
            }

            LibraryCombo {
                id: newKind

                model: [
                    {
                        name: "Lyrics",
                        kind: "song"
                    },
                    {
                        name: "Bilingual lyrics",
                        kind: "bilingual"
                    },
                    {
                        name: "Scripture",
                        kind: "scripture"
                    }
                ]
                textRole: "name"
                Layout.fillWidth: true
            }

            Text {
                text: panel.library.status
                color: "#AAAAAA"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                Item {
                    Layout.fillWidth: true
                }

                StudioButton {
                    text: "Cancel"

                    onClicked: createDialog.close()
                }

                StudioButton {
                    text: "Create & edit"
                    accented: true

                    onClicked: {
                        let id = panel.library.createTemplate(newName.text,
                                                              newKind.model[newKind.currentIndex].kind);
                        if (id) {
                            createDialog.close();
                            panel.editRequested(panel.library.layout(id));
                            panel.close();
                        }
                    }
                }
            }
        }

        onOpened: {
            newName.text = "My template";
            newName.forceActiveFocus();
            newName.selectAll();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        RowLayout {
            Column {
                spacing: 5

                Text {
                    text: panel.programTarget ? "Choose a layout" : "Templates"
                    color: "#EEEEEE"
                    font.pixelSize: 21
                    font.weight: Font.Medium
                }

                Text {
                    text: panel.programTarget
                          ? "Preview this service item before applying a new layout." :
                            "Design once. Reuse for lyrics, translations, and scripture."
                    color: "#929292"
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StudioButton {
                text: "New template"
                iconName: "plus"

                onClicked: createDialog.open()
            }

            StudioButton {
                text: "Import"

                onClicked: importFile.open()
            }

            StudioButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        RowLayout {
            visible: !panel.programTarget

            Repeater {
                model: [
                    {
                        title: "All",
                        kind: "all"
                    },
                    {
                        title: "Lyrics",
                        kind: "song"
                    },
                    {
                        title: "Bilingual",
                        kind: "bilingual"
                    },
                    {
                        title: "Scripture",
                        kind: "scripture"
                    }
                ]

                StudioButton {
                    required property var modelData

                    text: modelData.title
                    checked: panel.filter === modelData.kind

                    onClicked: panel.filter = modelData.kind
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            ListView {
                Layout.preferredWidth: 235
                Layout.maximumWidth: 260
                Layout.fillHeight: true
                clip: true
                spacing: 6
                model: panel.records

                ScrollBar.vertical: ScrollBar {
                }
                delegate: ItemDelegate {
                    id: record

                    required property var modelData

                    width: ListView.view.width
                    height: 76
                    Accessible.name: modelData.name

                    background: Rectangle {
                        color: panel.selectedId === record.modelData.id ? "#323232" :
                                                                          record.hovered
                                                                          ? "#242424" : "#202020"
                        radius: 7
                        border.color: "#363636"
                    }
                    contentItem: Column {
                        spacing: 7

                        Text {
                            width: parent.width
                            text: record.modelData.name
                            elide: Text.ElideRight
                            color: "#DDDDDD"
                            font.pixelSize: 13
                        }

                        Text {
                            text: record.modelData.kind + " · revision " + record.modelData.revision
                            color: "#888888"
                            font.pixelSize: 11
                        }
                    }

                    onClicked: panel.selectedId = modelData.id
                }
            }

            Rectangle {
                implicitWidth: 1
                Layout.fillHeight: true
                color: "#333333"
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                RowLayout {
                    Text {
                        text: panel.definition.name || "No template selected"
                        color: "#DDDDDD"
                        font.pixelSize: 16
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    StudioButton {
                        text: "Edit layout"
                        enabled: !!panel.selectedId

                        onClicked: {
                            panel.editRequested(panel.definition);
                            panel.close();
                        }
                    }
                }

                Rectangle {
                    id: frame

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 180
                    color: "#111111"
                    border.color: "#383838"
                    radius: 6
                    clip: true

                    SlideRenderer {
                        width: 1920
                        height: 1080
                        scale: Math.min(frame.width / 1920, frame.height / 1080)
                        transformOrigin: Item.TopLeft
                        x: (frame.width - width * scale) / 2
                        y: (frame.height - height * scale) / 2
                        elements: panel.previewSlide.elements || []
                        backgroundColor: panel.previewSlide.background || "#111111"
                        ambientOpacity: panel.previewSlide.ambientOpacity === undefined ? 0.5 :
                                                                                          panel.previewSlide.ambientOpacity
                        videoEnabled: false
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !panel.result.slides.length
                        text: "No preview available"
                        color: "#777777"
                        font.pixelSize: 13
                    }
                }

                RowLayout {
                    Text {
                        text: panel.programTarget ? "Service content preview" :
                                                    "Sample content preview"
                        color: "#888888"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }

                    StudioButton {
                        text: "Previous"
                        enabled: panel.previewIndex > 0

                        onClicked: panel.previewIndex--
                    }

                    Text {
                        text: panel.result.slides.length ? (panel.previewIndex + 1) + " / "
                                                           + panel.result.slides.length : "0 / 0"
                        color: "#AAAAAA"
                        font.pixelSize: 11
                    }

                    StudioButton {
                        text: "Next"
                        enabled: panel.previewIndex < panel.result.slides.length - 1

                        onClicked: panel.previewIndex++
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: panel.result.error
                          || "Text splits to fit the template's boxes, fonts, and selected line limit."
                    color: panel.result.error ? "#E1AD80" : "#999999"
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: !!panel.result.missingMedia?.length
                    text: "Missing media: " + (panel.result.missingMedia || []).join(", ")
                    color: "#E1AD80"
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: "#333333"
        }

        RowLayout {
            Text {
                text: panel.library.status
                color: "#999999"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            StudioButton {
                text: "Export"
                enabled: !!panel.selectedId

                onClicked: exportFile.open()
            }

            StudioButton {
                objectName: "applyLayoutButton"
                text: "Apply layout"
                accented: true
                visible: !!panel.programTarget
                enabled: panel.result.slides.length > 0 && !panel.result.error

                onClicked: {
                    if (panel.programTarget.applyLayout(panel.definition))
                        panel.close();
                    else
                        panel.result = {
                            slides: [],
                            error: panel.programTarget.status
                        };
                }
            }
        }
    }
}
