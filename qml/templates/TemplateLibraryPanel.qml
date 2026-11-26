pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Popup {
    id: panel

    property bool workspaceMode: false
    required property var library
    property var programTarget: null
    property string selectedId: ""
    property string filter: "all"
    property int previewIndex: 0
    property var result: ({
                              slides: [],
                              error: ""
                          })
    property string searchQuery: ""
    property bool inspecting: false
    readonly property var records: library.layouts.filter(record => {
        const kindMatches = filter === "all" || record.kind === filter || filter === "song"
              && record.kind === "bilingual";
        return kindMatches && record.name.toLowerCase().includes(searchQuery.trim().toLowerCase());
    })
    readonly property var definition: library.layouts.find(record => record.id === selectedId) || (
                                          {})
    readonly property var previewSlide: result.slides[previewIndex] || ({})

    signal editRequested(var definition)
    signal useRequested(var definition)

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

    closePolicy: workspaceMode ? Popup.NoAutoClose : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: workspaceMode ? parent.width : Math.min(1100, parent.width - 40)
    height: workspaceMode ? parent.height : Math.min(740, parent.height - 40)
    x: workspaceMode ? 0 : (parent.width - width) / 2
    y: workspaceMode ? 0 : (parent.height - height) / 2
    modal: !workspaceMode
    focus: true
    padding: LibraryStyle.pagePadding

    background: SquircleSurface {
        color: LibraryStyle.panel
        border.width: panel.workspaceMode ? 0 : 1
        border.color: LibraryStyle.tone("#3B3B3B")
        radius: panel.workspaceMode ? 0 : 12
    }

    onOpened: {
        filter = programTarget ? programTarget.layoutKind : "all";
        searchQuery = "";
        inspecting = false;
        refreshPreview();
    }
    onSelectedIdChanged: refreshPreview()
    onFilterChanged: refreshPreview()
    onSearchQueryChanged: refreshPreview()

    Connections {
        function onChanged() {
            panel.refreshPreview();
        }

        target: panel.library
    }

    FileDialog {
        id: importFile

        title: "Import template"
        nameFilters: ["Selah templates (*.churchtemplate)"]

        onAccepted: panel.library.importTemplate(selectedFile)
    }

    FileDialog {
        id: exportFile

        title: "Export template"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "churchtemplate"
        nameFilters: ["Selah templates (*.churchtemplate)"]

        onAccepted: panel.library.exportTemplate(panel.selectedId, selectedFile)
    }

    Dialog {
        id: createDialog

        title: "New template"
        modal: true
        anchors.centerIn: parent
        width: 360

        background: SquircleSurface {
            color: LibraryStyle.tone("#252525")
            border.color: LibraryStyle.tone("#444444")
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
                        name: "Presentation",
                        kind: "presentation"
                    },
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
                font.family: LibraryStyle.fontFamily
                text: panel.library.status
                color: LibraryStyle.tone("#AAAAAA")
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                Item {
                    Layout.fillWidth: true
                }

                LibraryButton {
                    text: "Cancel"

                    onClicked: createDialog.close()
                }

                LibraryButton {
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
        spacing: 16

        RowLayout {
            Column {
                spacing: 5

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: panel.programTarget ? "Choose a layout" : "Templates"
                    color: LibraryStyle.tone("#EEEEEE")
                    font.pixelSize: 22
                    font.weight: Font.Medium
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: panel.programTarget
                          ? "Preview this service item before applying a new layout." :
                            "Reusable layouts for worship, messages, lyrics, and scripture."
                    color: LibraryStyle.tone("#929292")
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "New template"
                iconName: "plus"

                onClicked: createDialog.open()
            }

            LibraryButton {
                text: "Import"

                onClicked: importFile.open()
            }

            LibraryButton {
                text: "Close"
                visible: !panel.workspaceMode
                quiet: true

                onClicked: panel.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryButton {
                text: "All layouts"
                iconName: "chevron-left"
                visible: panel.inspecting

                onClicked: panel.inspecting = false
            }

            LibraryField {
                objectName: "templateSearch"
                Layout.fillWidth: true
                leadingIcon: "search"
                placeholderText: "Search templates…"
                text: panel.searchQuery

                onTextEdited: {
                    panel.inspecting = false;
                    panel.searchQuery = text;
                }
            }

            Text {
                text: panel.records.length + " templates"
                color: LibraryStyle.muted
                font.pixelSize: 11
            }
        }

        LibraryEmptyState {
            visible: !panel.records.length
            Layout.fillWidth: true
            title: "No matching templates"
            description: "Try another name or choose a different category."
            iconName: "search"
        }

        LibraryTabs {
            visible: !panel.programTarget
            options: [
                {
                    text: "All",
                    value: "all"
                },
                {
                    text: "Presentation",
                    value: "presentation"
                },
                {
                    text: "Lyrics",
                    value: "song"
                },
                {
                    text: "Bilingual",
                    value: "bilingual"
                },
                {
                    text: "Scripture",
                    value: "scripture"
                }
            ]
            currentValue: panel.filter

            onChosen: value => panel.filter = value
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            ScrollView {
                id: templateScroll

                visible: !panel.inspecting
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                GridLayout {
                    id: templateGrid

                    width: templateScroll.availableWidth
                    columns: Math.max(1, Math.floor(width / 230))
                    columnSpacing: 12
                    rowSpacing: 12

                    Repeater {
                        model: panel.records

                        delegate: ItemDelegate {
                            id: card

                            required property var modelData

                            objectName: "templateCard_" + modelData.id
                            Layout.fillWidth: true
                            Layout.preferredWidth: (templateGrid.width - (templateGrid.columns - 1)
                                                    * 12) / templateGrid.columns
                            Layout.maximumWidth: Layout.preferredWidth
                            implicitHeight: width * 9 / 16 + 54
                            padding: 8
                            hoverEnabled: true
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: modelData.name + ", " + modelData.kind

                            background: SquircleSurface {
                                radius: 7
                                color: card.hovered ? LibraryStyle.hover : LibraryStyle.field
                                border.color: panel.selectedId === card.modelData.id
                                              ? LibraryStyle.focus : LibraryStyle.border
                            }
                            contentItem: Column {
                                // Preview TextEdits must not consume the card's pointer events.
                                enabled: false
                                spacing: 5

                                Rectangle {
                                    id: thumbnail

                                    width: parent.width
                                    height: width * 9 / 16
                                    color: "#111111"
                                    clip: true

                                    SlideRenderer {
                                        showTransparencyGrid: true
                                        scale: thumbnail.width / 1920
                                        transformOrigin: Item.TopLeft
                                        elements: card.modelData.slide.elements || []
                                        backgroundFill: card.modelData.slide.backgroundFill || ({})
                                        backgroundColor: card.modelData.slide.background
                                                         || "#111111"
                                        ambientOpacity: card.modelData.slide.ambientOpacity || 0
                                        videoEnabled: false
                                    }
                                }

                                Text {
                                    width: parent.width
                                    text: card.modelData.name
                                    elide: Text.ElideRight
                                    font.family: LibraryStyle.fontFamily
                                    font.pixelSize: 12
                                    color: LibraryStyle.text
                                }

                                Text {
                                    text: card.modelData.kind + " · revision "
                                          + card.modelData.revision
                                    font.family: LibraryStyle.fontFamily
                                    font.pixelSize: 10
                                    color: LibraryStyle.muted
                                }
                            }

                            onClicked: {
                                panel.selectedId = modelData.id;
                                if (panel.programTarget) {
                                    panel.inspecting = true;
                                } else {
                                    panel.editRequested(modelData);
                                    panel.close();
                                }
                            }

                            HoverHandler {
                                cursorShape: Qt.PointingHandCursor
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                visible: panel.inspecting
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 12

                RowLayout {
                    Text {
                        font.family: LibraryStyle.fontFamily
                        text: panel.definition.name || "No template selected"
                        color: LibraryStyle.tone("#DDDDDD")
                        font.pixelSize: 16
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    LibraryButton {
                        text: "Use template"
                        visible: !panel.programTarget && panel.definition.kind === "presentation"
                        enabled: !!panel.selectedId
                        accented: true

                        onClicked: {
                            panel.useRequested(panel.definition);
                            panel.close();
                        }
                    }

                    LibraryButton {
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
                    color: LibraryStyle.tone("#111111")
                    border.color: LibraryStyle.tone("#383838")
                    radius: 6
                    clip: true

                    SlideRenderer {
                        showTransparencyGrid: true
                        width: 1920
                        height: 1080
                        scale: Math.min(frame.width / 1920, frame.height / 1080)
                        transformOrigin: Item.TopLeft
                        x: (frame.width - width * scale) / 2
                        y: (frame.height - height * scale) / 2
                        elements: panel.previewSlide.elements || []
                        backgroundFill: panel.previewSlide.backgroundFill || ({})
                        backgroundColor: panel.previewSlide.background || LibraryStyle.tone(
                                             "#111111")
                        ambientOpacity: panel.previewSlide.ambientOpacity === undefined ? 0.5 :
                                                                                          panel.previewSlide.ambientOpacity
                        videoEnabled: false
                    }

                    Text {
                        font.family: LibraryStyle.fontFamily
                        anchors.centerIn: parent
                        visible: !panel.result.slides.length
                        text: "No preview available"
                        color: LibraryStyle.tone("#777777")
                        font.pixelSize: 13
                    }
                }

                RowLayout {
                    Text {
                        font.family: LibraryStyle.fontFamily
                        text: panel.programTarget ? "Service content preview" :
                                                    panel.definition.kind === "presentation"
                                                    ? "Editable layout preview" :
                                                      "Sample content preview"
                        color: LibraryStyle.tone("#888888")
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }

                    LibraryButton {
                        text: "Previous"
                        enabled: panel.previewIndex > 0

                        onClicked: panel.previewIndex--
                    }

                    Text {
                        font.family: LibraryStyle.fontFamily
                        text: panel.result.slides.length ? (panel.previewIndex + 1) + " / "
                                                           + panel.result.slides.length : "0 / 0"
                        color: LibraryStyle.tone("#AAAAAA")
                        font.pixelSize: 11
                    }

                    LibraryButton {
                        text: "Next"
                        enabled: panel.previewIndex < panel.result.slides.length - 1

                        onClicked: panel.previewIndex++
                    }
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    Layout.fillWidth: true
                    text: panel.result.error || (panel.definition.kind === "presentation"
                                                 ? "Use this layout to start a slide. Edit layout updates the reusable template." :
                                                   "Text splits to fit the template's boxes, fonts, and selected line limit.")
                    color: panel.result.error ? "#E1AD80" : LibraryStyle.tone("#999999")
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }

                Text {
                    font.family: LibraryStyle.fontFamily
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
            color: LibraryStyle.tone("#333333")
        }

        RowLayout {
            Text {
                font.family: LibraryStyle.fontFamily
                text: panel.library.status
                color: LibraryStyle.tone("#999999")
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            LibraryButton {
                text: "Export"
                visible: panel.inspecting
                enabled: !!panel.selectedId

                onClicked: exportFile.open()
            }

            LibraryButton {
                objectName: "applyLayoutButton"
                text: "Apply layout"
                accented: true
                visible: !!panel.programTarget && panel.inspecting
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
