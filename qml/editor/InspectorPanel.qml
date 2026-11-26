pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Property controls call the host command interface instead of owning a second document copy.
StudioPanel {
    id: inspector

    required property var host
    property bool collapsed: false
    readonly property bool canvasSelected: host.doc.selected < 0

    width: 252
    height: collapsed ? 50 : Math.max(50, parent.height - 44)
    clip: true

    Behavior on height {
        NumberAnimation {
            duration: InteractionFeedback.duration
        }
    }

    Connections {
        function onCanvasSettingsRequested() {
            inspector.collapsed = false;
            const viewport = inspectorScroll.contentItem as Flickable;
            if (viewport)
                viewport.contentY = 0;
        }

        target: inspector.host
    }

    anchors {
        right: parent.right
        top: parent.top
        rightMargin: 18
        topMargin: 20
        bottomMargin: 24
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12

            Text {
                text: "Properties"
                color: LibraryStyle.tone("#E0E0E0")
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            Text {
                text: inspector.host.doc.selected < 0 ? "Canvas" : inspector.host.selectedValue(
                                                            "kind", "text") + " layer"
                color: LibraryStyle.tone("#777777")
                font.pixelSize: 10
            }

            StudioButton {
                objectName: "togglePropertiesPanel"
                iconName: inspector.collapsed ? "chevron-down" : "chevron-up"
                quiet: true
                implicitWidth: 24
                implicitHeight: 26
                hint: inspector.collapsed ? "Expand Properties" : "Collapse Properties"
                Accessible.name: hint

                onClicked: inspector.collapsed = !inspector.collapsed
            }
        }

        ColumnLayout {
            objectName: "propertiesPanelBody"
            visible: !inspector.collapsed
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            StudioRule {
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 10
                spacing: 7

                StudioButton {
                    text: "Import"
                    iconName: "download"
                    Layout.fillWidth: true
                    implicitHeight: 30

                    onClicked: inspector.host.openPresentation()
                }

                StudioButton {
                    text: "Export"
                    iconName: "upload"
                    Layout.fillWidth: true
                    implicitHeight: 30

                    onClicked: inspector.host.exportPresentation()
                }
            }

            StudioRule {
            }

            ScrollView {
                id: inspectorScroll

                objectName: "inspectorPropertyScroll"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                contentWidth: availableWidth

                ColumnLayout {
                    width: inspector.width - 26
                    x: 12
                    spacing: 12

                    CanvasBackgroundControls {
                        visible: inspector.canvasSelected
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        host: inspector.host
                    }

                    StudioRule {
                        visible: inspector.canvasSelected
                    }

                    EditorTransitions {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        visible: inspector.canvasSelected && !inspector.host.templateMode
                        document: inspector.host.doc
                    }

                    StudioRule {
                        visible: inspector.canvasSelected && !inspector.host.templateMode
                    }

                    StudioSectionHeading {
                        id: transformSection

                        label: "TRANSFORM"
                        Layout.topMargin: 8

                        onResetRequested: {
                            inspector.host.update("rotation", 0);
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: transformSection.expanded

                        GridLayout {
                            columns: 2
                            Layout.fillWidth: true
                            columnSpacing: 8
                            rowSpacing: 8
                            enabled: inspector.host.doc.selected >= 0

                            InspectorField {
                                Layout.fillWidth: true
                                caption: "X"
                                minimumValue: 0
                                maximumValue: 1880
                                value: Math.round(inspector.host.selectedValue("x", 0))

                                onCommitted: value => inspector.host.update("x", Number(value))
                            }

                            InspectorField {
                                Layout.fillWidth: true
                                caption: "Y"
                                minimumValue: 0
                                maximumValue: 1050
                                value: Math.round(inspector.host.selectedValue("y", 0))

                                onCommitted: value => inspector.host.update("y", Number(value))
                            }

                            InspectorField {
                                Layout.fillWidth: true
                                caption: "W"
                                minimumValue: 40
                                maximumValue: 1920
                                value: Math.round(inspector.host.selectedValue("width", 1920))

                                onCommitted: value => inspector.host.update("width", Number(value))
                            }

                            InspectorField {
                                Layout.fillWidth: true
                                caption: "H"
                                minimumValue: 30
                                maximumValue: 1080
                                value: Math.round(inspector.host.selectedValue("height", 1080))

                                onCommitted: value => inspector.host.update("height", Number(value))
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Rotation"
                                color: LibraryStyle.tone("#929292")
                                font.pixelSize: 11
                                Layout.fillWidth: true
                            }

                            InspectorField {
                                Layout.preferredWidth: 92
                                caption: "°"
                                minimumValue: -360
                                maximumValue: 360
                                value: inspector.host.selectedValue("rotation", 0)
                                enabled: inspector.host.doc.selected >= 0

                                onCommitted: value => inspector.host.update("rotation", Number(
                                                                                value))
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Snap to grid"
                                color: LibraryStyle.tone("#929292")
                                font.pixelSize: 11
                                Layout.fillWidth: true
                            }

                            Switch {
                                id: snapSwitch

                                checked: inspector.host.snap
                                implicitWidth: 31
                                implicitHeight: 24

                                indicator: Rectangle {
                                    width: 28
                                    height: 16
                                    y: 4
                                    radius: 8
                                    color: snapSwitch.checked ? LibraryStyle.tone("#DEDEDE") :
                                                                LibraryStyle.tone("#4A4A4A")

                                    Rectangle {
                                        width: 12
                                        height: 12
                                        y: 2
                                        x: snapSwitch.checked ? 14 : 2
                                        radius: 6
                                        color: snapSwitch.checked ? LibraryStyle.tone("#252525") :
                                                                    LibraryStyle.tone("#D0D0D0")

                                        Behavior on x {
                                            NumberAnimation {
                                                duration: 120
                                            }
                                        }
                                    }
                                }

                                onToggled: inspector.host.snap = checked
                            }
                        }
                    }

                    StudioRule {
                    }

                    StudioSectionHeading {
                        id: mediaSection

                        label: "MEDIA"
                        visible: inspector.host.selectedValue("kind", "text") !== "text"

                        onResetRequested: inspector.host.update("fit", "contain")
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        visible: mediaSection.visible && mediaSection.expanded

                        Text {
                            text: inspector.host.selectedValue("name", "")
                            color: LibraryStyle.tone("#AAAAAA")
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        StudioSelect {
                            Layout.fillWidth: true
                            model: ["Contain", "Cover", "Stretch"]
                            currentIndex: ["contain", "cover", "stretch"].indexOf(
                                inspector.host.selectedValue("fit", "contain"))

                            onActivated: inspector.host.update("fit", currentText.toLowerCase())
                        }

                        StudioButton {
                            text: "Fill slide"
                            Layout.fillWidth: true

                            onClicked: inspector.host.doc.updateElement(inspector.host.doc.selected,
                                                                        {
                                                                            x: 0,
                                                                            y: 0,
                                                                            width: 1920,
                                                                            height: 1080,
                                                                            fit: "cover"
                                                                        })
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: ["video", "audio"].includes(inspector.host.selectedValue("kind",
                                                                                              ""))

                            StudioButton {
                                text: "Loop"
                                checked: inspector.host.selectedValue("loop", true)
                                Layout.fillWidth: true

                                onClicked: inspector.host.update("loop",
                                                                 !inspector.host.selectedValue(
                                                                     "loop", true))
                            }

                            StudioButton {
                                text: "Muted"
                                checked: inspector.host.selectedValue("muted", true)
                                Layout.fillWidth: true

                                onClicked: inspector.host.update("muted",
                                                                 !inspector.host.selectedValue(
                                                                     "muted", true))
                            }
                        }

                        Text {
                            text: "Editor playback is always muted.\nEnable preview audio separately."
                            visible: ["video", "audio"].includes(inspector.host.selectedValue("kind",
                                                                                              ""))
                            color: LibraryStyle.tone("#888888")
                            font.pixelSize: 10
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                        }
                    }

                    StudioRule {
                        visible: mediaSection.visible
                    }

                    StudioSectionHeading {
                        id: typeSection

                        visible: inspector.host.selectedValue("kind", "text") === "text"
                        label: "TYPOGRAPHY"

                        onResetRequested: {
                            inspector.host.update("fontFamily", "Georgia");
                            inspector.host.update("fontSize", 174);
                        }
                    }

                    TextInspector {
                        objectName: "textInspector"
                        Layout.fillWidth: true
                        visible: typeSection.visible && typeSection.expanded
                        enabled: inspector.host.doc.selected >= 0
                        host: inspector.host

                        onChooseFont: {
                            fontPicker.currentFamily = inspector.host.selectedValue("fontFamily",
                                                                                    "Arial");
                            fontPicker.targetId = inspector.host.doc.selectedElement.uid || "";
                            const editor = inspector.host.activeTextEditor;
                            const selected = editor && editor.selectionTo > editor.selectionFrom
                                  ? editor.currentElement().text.slice(editor.selectionFrom,
                                                                       editor.selectionTo) : "";
                            fontPicker.previewText = selected
                                    || inspector.host.doc.selectedElement.text || "";
                            fontPicker.open();
                        }
                    }

                    StudioRule {
                    }

                    StudioSectionHeading {
                        id: appearanceSection

                        label: "APPEARANCE"

                        onResetRequested: inspector.host.update("opacity", 1)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        visible: appearanceSection.expanded

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Opacity"
                                color: LibraryStyle.tone("#929292")
                                font.pixelSize: 11
                                Layout.fillWidth: true
                            }

                            Text {
                                text: Math.round(opacitySlider.value) + "%"
                                color: LibraryStyle.tone("#AEAEAE")
                                font.pixelSize: 11
                            }
                        }

                        Slider {
                            id: opacitySlider

                            Layout.fillWidth: true
                            implicitHeight: 23
                            from: 0
                            to: 100
                            value: inspector.host.selectedValue("opacity", 1) * 100
                            enabled: inspector.host.doc.selected >= 0

                            background: Rectangle {
                                x: opacitySlider.leftPadding
                                y: opacitySlider.topPadding + opacitySlider.availableHeight / 2
                                   - height / 2
                                width: opacitySlider.availableWidth
                                height: 1
                                color: LibraryStyle.tone("#474747")

                                Rectangle {
                                    width: opacitySlider.visualPosition * parent.width
                                    height: 1
                                    color: LibraryStyle.tone("#D7D7D7")
                                }
                            }
                            handle: Rectangle {
                                x: opacitySlider.leftPadding + opacitySlider.visualPosition * (
                                       opacitySlider.availableWidth - width)
                                y: opacitySlider.topPadding + opacitySlider.availableHeight / 2
                                   - height / 2
                                width: 8
                                height: 8
                                radius: 2
                                color: opacitySlider.pressed ? LibraryStyle.tone("#FFFFFF") :
                                                               LibraryStyle.tone("#DDDDDD")
                            }

                            onPressedChanged: if (!pressed)
                                                  inspector.host.update("opacity", value / 100)
                        }
                    }

                    StudioRule {
                        visible: inspector.host.templateMode
                    }

                    StudioSectionHeading {
                        id: bindingSection

                        label: "CONTENT BINDING"
                        visible: inspector.host.templateMode && inspector.host.selectedValue("kind",
                                                                                             "text")
                                 === "text"

                        onResetRequested: inspector.host.update("binding", "None")
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: bindingSection.visible && bindingSection.expanded
                        spacing: 9

                        StudioSelect {
                            Layout.fillWidth: true
                            model: ["None", "Title", "Section name", "Credits", "Version",
                                "Primary lyrics", "Translated lyrics", "Scripture text",
                                "Scripture reference", "Secondary scripture text",
                                "Secondary scripture reference", "Secondary version",
                                "Secondary credits"]
                            currentIndex: Math.max(0, model.indexOf(inspector.host.selectedValue(
                                                                        "binding", "None")))

                            onActivated: inspector.host.update("binding", currentText)
                        }

                        Text {
                            text: "Save this slide as a template.\nBound text is filled and split automatically."
                            color: LibraryStyle.tone("#777777")
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Item {
                        implicitHeight: 12
                    }
                }
            }

            StudioRule {
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 12

                Rectangle {
                    implicitWidth: 5
                    implicitHeight: 5
                    radius: 3
                    color: LibraryStyle.tone("#AFAFAF")
                }

                Text {
                    text: "Local project"
                    color: LibraryStyle.tone("#888888")
                    font.pixelSize: 10
                    Layout.fillWidth: true
                }

                Text {
                    text: "Draft"
                    color: LibraryStyle.tone("#B7B7B7")
                    font.pixelSize: 10
                }
            }
        }
    }

    FontPicker {
        id: fontPicker

        objectName: "fontPicker"
        parent: Overlay.overlay
        catalog: inspector.host.fonts

        onOpened: inspector.host.fontPickerOpen = true
        onClosed: inspector.host.fontPickerOpen = false
        onChosen: (family, elementId) => {
            if (inspector.host.doc.selectedElement.uid === elementId) {
                inspector.host.update("fontFamily", family);
                inspector.host.notify("Font applied: " + family);
            } else
                inspector.host.notify("Font is ready. Select a text layer to apply it.");
        }
    }
}
