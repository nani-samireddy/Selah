pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ChurchDeck.Formatting

ColumnLayout {
    id: inspector

    required property var host
    property bool advanced: false

    signal chooseFont

    function value(key, fallback) {
        return host.selectedValue(key, fallback);
    }

    function set(key, value) {
        if (typeof value === "number" && !Number.isFinite(value))
            return;
        host.update(key, value);
    }

    function setLineHeightUnit(unit) {
        if (unit === value("lineHeightUnit", "percent"))
            return;
        const old = value("lineHeight", 1.0), size = Math.max(8, value("fontSize", 36));
        host.formatText({
                            lineHeightUnit: unit,
                            lineHeight: unit === "px" ? old * size : old / size
                        });
    }

    function setFeature(tag, enabled) {
        let features = Object.assign({}, value("fontFeatures", {}));
        features[tag] = enabled ? 1 : 0;
        set("fontFeatures", features);
    }

    function setAxis(tag, amount) {
        let axes = Object.assign({}, value("fontAxes", {}));
        axes[tag] = amount;
        set("fontAxes", axes);
    }

    spacing: 8

    LibraryCaption {
        text: inspector.host.activeTextEditor && inspector.host.activeTextEditor.selectionFrom
              !== inspector.host.activeTextEditor.selectionTo ? "Selected text" : "Text layer"
    }

    LibraryButton {
        objectName: "textFontFamily"
        Layout.fillWidth: true
        text: inspector.value("fontFamily", "Arial")
        iconName: "type"
        hint: "Choose font · " + text

        onClicked: inspector.chooseFont()
    }

    RowLayout {
        Layout.fillWidth: true

        LibraryCombo {
            objectName: "textFontWeight"
            Layout.fillWidth: true
            model: [
                {
                    label: "Thin",
                    value: 100
                },
                {
                    label: "Extra light",
                    value: 200
                },
                {
                    label: "Light",
                    value: 300
                },
                {
                    label: "Regular",
                    value: 400
                },
                {
                    label: "Medium",
                    value: 500
                },
                {
                    label: "Semibold",
                    value: 600
                },
                {
                    label: "Bold",
                    value: 700
                },
                {
                    label: "Extra bold",
                    value: 800
                },
                {
                    label: "Black",
                    value: 900
                }
            ]
            textRole: "label"
            valueRole: "value"
            currentIndex: model.findIndex(row => row.value === inspector.value("fontWeight", 400))
            Accessible.name: "Font weight"

            onActivated: inspector.set("fontWeight", currentValue)
        }

        InspectorField {
            Layout.preferredWidth: 75
            caption: "px"
            minimumValue: 8
            maximumValue: 400
            value: inspector.value("fontSize", 36)

            onCommitted: value => inspector.set("fontSize", Number(value))
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Repeater {
            model: [
                {
                    label: "B",
                    key: "fontWeight",
                    hint: "Bold"
                },
                {
                    label: "I",
                    key: "italic",
                    hint: "Italic"
                },
                {
                    label: "U",
                    key: "underline",
                    hint: "Underline"
                },
                {
                    label: "S",
                    key: "strikethrough",
                    hint: "Strikethrough"
                }
            ]

            LibraryButton {
                required property var modelData

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: modelData.label
                hint: modelData.hint
                selected: modelData.key === "fontWeight" ? inspector.value("fontWeight", 400) >= 600 :
                                                           inspector.value(modelData.key, false)

                onClicked: inspector.set(modelData.key, modelData.key === "fontWeight" ? (selected
                                                                                          ? 400 : 700) :
                                                                                         !selected)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true

        TypeField {
            id: lineHeightField

            label: "Line height"
            unitSelectable: true
            caption: inspector.value("lineHeightUnit", "percent") === "px" ? "px" : "%"
            value: inspector.value("lineHeightUnit", "percent") === "px" ? inspector.value(
                                                                               "lineHeight", 1.0) :
                                                                           Math.round(
                                                                               inspector.value(
                                                                                   "lineHeight",
                                                                                   1.0) * 100)

            onCommitted: amount => inspector.set("lineHeight", inspector.value("lineHeightUnit",
                                                                               "percent") === "px"
                                                 ? amount : amount / 100)
            onUnitClicked: lineUnitMenu.popup(lineHeightField, 0, lineHeightField.height + 4)

            Menu {
                id: lineUnitMenu

                objectName: "lineHeightUnitMenu"
                popupType: Popup.Item
                implicitWidth: 188
                padding: 5

                background: SquircleSurface {
                    radius: 8
                    color: LibraryStyle.panel
                    border.color: LibraryStyle.border
                }

                MenuItem {
                    id: percentOption

                    objectName: "lineHeightPercentOption"
                    implicitHeight: 30
                    autoExclusive: true
                    text: "Percent (%)"
                    checkable: true
                    checked: inspector.value("lineHeightUnit", "percent") === "percent"

                    contentItem: Text {
                        text: percentOption.text
                        color: LibraryStyle.text
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 12
                        leftPadding: 20
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Text {
                        text: percentOption.checked ? "•" : ""
                        x: 8
                        anchors.verticalCenter: parent.verticalCenter
                        color: LibraryStyle.text
                    }
                    background: SquircleSurface {
                        radius: 5
                        color: percentOption.highlighted ? LibraryStyle.hover : "transparent"
                    }

                    onTriggered: inspector.setLineHeightUnit("percent")
                }

                MenuItem {
                    id: pixelsOption

                    objectName: "lineHeightPixelsOption"
                    implicitHeight: 30
                    autoExclusive: true
                    text: "Pixels (px)"
                    checkable: true
                    checked: inspector.value("lineHeightUnit", "percent") === "px"

                    contentItem: Text {
                        text: pixelsOption.text
                        color: LibraryStyle.text
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 12
                        leftPadding: 20
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Text {
                        text: pixelsOption.checked ? "•" : ""
                        x: 8
                        anchors.verticalCenter: parent.verticalCenter
                        color: LibraryStyle.text
                    }
                    background: SquircleSurface {
                        radius: 5
                        color: pixelsOption.highlighted ? LibraryStyle.hover : "transparent"
                    }

                    onTriggered: inspector.setLineHeightUnit("px")
                }
            }
        }

        TypeField {
            label: "Letter spacing"
            caption: "px"
            value: inspector.value("letterSpacing", 0)

            onCommitted: amount => inspector.set("letterSpacing", amount)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        TextOptionGroup {
            controller: inspector.host
            propertyName: "alignment"
            options: [
                {
                    value: "left",
                    label: "Align left",
                    path: "M4 5h16M4 10h10M4 15h16M4 20h10"
                },
                {
                    value: "center",
                    label: "Align center",
                    path: "M4 5h16M7 10h10M4 15h16M7 20h10"
                },
                {
                    value: "right",
                    label: "Align right",
                    path: "M4 5h16M10 10h10M4 15h16M10 20h10"
                },
                {
                    value: "justify",
                    label: "Justify text",
                    path: "M4 5h16M4 10h16M4 15h16M4 20h16"
                }
            ]
        }

        TextOptionGroup {
            controller: inspector.host
            propertyName: "verticalAlignment"
            options: [
                {
                    value: "top",
                    label: "Align top",
                    path: "M4 4h16M7 9h10M7 13h10M7 17h10"
                },
                {
                    value: "center",
                    label: "Align middle",
                    path: "M4 12h16M7 5h10M7 19h10"
                },
                {
                    value: "bottom",
                    label: "Align bottom",
                    path: "M4 20h16M7 7h10M7 11h10M7 15h10"
                }
            ]
        }
    }

    LibraryCombo {
        objectName: "textSizingCombo"
        Layout.fillWidth: true
        model: [
            {
                label: "Fixed text box",
                value: "fixed"
            },
            {
                label: "Auto height",
                value: "autoHeight"
            },
            {
                label: "Auto width",
                value: "autoWidth"
            },
            {
                label: "Shrink to fit",
                value: "shrink"
            }
        ]
        textRole: "label"
        valueRole: "value"
        currentIndex: model.findIndex(row => row.value === inspector.value("textSizing", "fixed"))
        Accessible.name: "Text resizing"

        onActivated: inspector.set("textSizing", currentValue)
    }

    RowLayout {
        visible: inspector.value("textSizing", "fixed") === "shrink"
        Layout.fillWidth: true

        LibraryCaption {
            text: "Minimum font size"
            Layout.fillWidth: true
        }

        InspectorField {
            objectName: "textMinimumFontSize"
            Layout.preferredWidth: 85
            caption: "px"
            minimumValue: 8
            maximumValue: inspector.value("fontSize", 36)
            value: Math.min(inspector.value("minFontSize", 32), maximumValue)

            onCommitted: value => inspector.set("minFontSize", Number(value))
        }
    }

    LibraryCaption {
        visible: inspector.value("textSizing", "fixed") === "shrink"
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: TextFormatting.display(inspector.host.doc.selectedElement)._fitOverflow
              ? "Text still overflows at the minimum size. Enlarge the box or lower the minimum." :
                "Font size is the maximum. Text shrinks only as needed; your original size is preserved."
    }

    ColorField {
        objectName: "textColorInput"
        Layout.fillWidth: true
        label: "Text color"
        value: inspector.value("color", "#FFFFFF")

        onPickerRequested: inspector.host.showTextPicker()
        onCommitted: value => inspector.set("color", value)
    }

    LibraryButton {
        text: inspector.advanced ? "Fewer text options" : "More text options"
        iconName: inspector.advanced ? "chevron-up" : "chevron-down"
        quiet: true
        Layout.fillWidth: true

        onClicked: inspector.advanced = !inspector.advanced
    }

    ColumnLayout {
        visible: inspector.advanced
        Layout.fillWidth: true
        spacing: 10

        LibraryCaption {
            text: "Case & paragraphs"
        }

        TextOptionGroup {
            controller: inspector.host
            propertyName: "textCase"
            options: [
                {
                    value: "original",
                    label: "Original case",
                    text: "—"
                },
                {
                    value: "upper",
                    label: "UPPERCASE",
                    text: "AA"
                },
                {
                    value: "lower",
                    label: "lowercase",
                    text: "aa"
                },
                {
                    value: "title",
                    label: "Title Case",
                    text: "Aa"
                },
                {
                    value: "smallCaps",
                    label: "Small caps",
                    text: "ᴀᴀ"
                }
            ]
        }

        RowLayout {
            Layout.fillWidth: true

            TypeField {
                label: "Paragraph gap"
                caption: "px"
                value: inspector.value("paragraphSpacing", 0)

                onCommitted: amount => inspector.set("paragraphSpacing", amount)
            }

            TypeField {
                label: "First-line indent"
                caption: "px"
                value: inspector.value("paragraphIndent", 0)

                onCommitted: amount => inspector.set("paragraphIndent", amount)
            }
        }

        LibraryCaption {
            text: "List style"
            font.pixelSize: 10
        }

        TextOptionGroup {
            controller: inspector.host
            propertyName: "listStyle"
            options: [
                {
                    value: "none",
                    label: "No list",
                    path: "M4 6h16M4 12h16M4 18h16"
                },
                {
                    value: "bullet",
                    label: "Bullet list",
                    path: "M4 6h.01M4 12h.01M4 18h.01M9 6h11M9 12h11M9 18h11"
                },
                {
                    value: "number",
                    label: "Numbered list",
                    path: "M3 4h1v5M3 9h3M2 13h4v3H2v4h4M10 6h10M10 12h10M10 18h10"
                }
            ]
        }

        TypeField {
            label: "List indent"
            value: inspector.value("listIndent", 1)

            onCommitted: amount => inspector.set("listIndent", amount)
        }

        LibraryCaption {
            text: "Baseline"
            font.pixelSize: 10
        }

        TextOptionGroup {
            controller: inspector.host
            propertyName: "baseline"
            options: [
                {
                    value: "normal",
                    label: "Normal baseline",
                    text: "x"
                },
                {
                    value: "super",
                    label: "Superscript",
                    text: "x²"
                },
                {
                    value: "sub",
                    label: "Subscript",
                    text: "x₂"
                }
            ]
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryButton {
                text: "Wrap lines"
                Layout.fillWidth: true
                selected: inspector.value("wrap", true)

                onClicked: inspector.set("wrap", !selected)
            }

            LibraryButton {
                text: "Clip overflow"
                Layout.fillWidth: true
                selected: inspector.value("clipText", true)

                onClicked: inspector.set("clipText", !selected)
            }
        }

        LibraryCaption {
            text: "OpenType features"
        }

        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            spacing: 4

            Repeater {
                model: [
                    {
                        label: "Ligatures",
                        tag: "liga"
                    },
                    {
                        label: "Kerning",
                        tag: "kern"
                    },
                    {
                        label: "Contextual",
                        tag: "calt"
                    },
                    {
                        label: "Discretionary",
                        tag: "dlig"
                    },
                    {
                        label: "Tabular numbers",
                        tag: "tnum"
                    },
                    {
                        label: "Fractions",
                        tag: "frac"
                    },
                    {
                        label: "Slashed zero",
                        tag: "zero"
                    },
                    {
                        label: "Ordinals",
                        tag: "ordn"
                    },
                    {
                        label: "Small capitals",
                        tag: "smcp"
                    }
                ]

                LibraryButton {
                    required property var modelData

                    text: modelData.label
                    labelFontSize: 11
                    implicitHeight: 28
                    selected: (inspector.value("fontFeatures", {})[modelData.tag] ?? (["liga", "kern",
                                                                                       "calt"].includes(
                                                                                          modelData.tag)
                                                                                      ? 1 : 0)) > 0

                    onClicked: inspector.setFeature(modelData.tag, !selected)
                }
            }
        }

        LibraryCaption {
            text: "Variable font axes"
        }

        RowLayout {
            Layout.fillWidth: true

            TypeField {
                label: "Weight · wght"
                value: inspector.value("fontAxes", {}).wght ?? inspector.value("fontWeight", 400)

                onCommitted: amount => inspector.setAxis("wght", amount)
            }

            TypeField {
                label: "Width · wdth"
                value: inspector.value("fontAxes", {}).wdth ?? 100

                onCommitted: amount => inspector.setAxis("wdth", amount)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TypeField {
                label: "Optical size · opsz"
                value: inspector.value("fontAxes", {}).opsz ?? inspector.value("fontSize", 36)

                onCommitted: amount => inspector.setAxis("opsz", amount)
            }

            TypeField {
                label: "Slant · slnt"
                value: inspector.value("fontAxes", {}).slnt ?? 0

                onCommitted: amount => inspector.setAxis("slnt", amount)
            }
        }

        LibraryCaption {
            text: "Custom feature or axis"
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryField {
                id: customTag

                Layout.fillWidth: true
                placeholderText: "ss01"
                maximumLength: 4
                Accessible.name: "OpenType tag"
            }

            InspectorField {
                id: customAmount

                Layout.preferredWidth: 70
                value: "1"

                onCommitted: value => {}
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryButton {
                text: "Set feature"
                Layout.fillWidth: true
                enabled: customTag.text.length === 4

                onClicked: {
                    let values = Object.assign({}, inspector.value("fontFeatures", {}));
                    values[customTag.text] = Number(customAmount.currentText);
                    inspector.set("fontFeatures", values);
                }
            }

            LibraryButton {
                text: "Set axis"
                Layout.fillWidth: true
                enabled: customTag.text.length === 4

                onClicked: inspector.setAxis(customTag.text, Number(customAmount.currentText))
            }
        }

        Text {
            text: "Available features and axes depend on the font."
            color: LibraryStyle.muted
            font.pixelSize: 10
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }

    component TextOptionGroup: Rectangle {
        id: group

        required property var controller
        required property string propertyName
        required property var options

        Layout.fillWidth: true
        Layout.preferredWidth: options.length * 30
        Layout.minimumWidth: options.length * 26 + 6
        implicitHeight: 34
        color: LibraryStyle.field
        radius: 6
        border.color: LibraryStyle.border

        RowLayout {
            anchors.fill: parent
            anchors.margins: 3
            spacing: 2

            Repeater {
                model: group.options

                LibraryButton {
                    id: stateButton

                    required property var modelData

                    objectName: "text_" + group.propertyName + "_" + modelData.value
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    implicitWidth: 24
                    implicitHeight: 28
                    checkable: true
                    autoExclusive: true
                    checked: group.controller.selectedValue(group.propertyName,
                                                            group.options[0].value)
                             === modelData.value
                    selected: checked
                    hint: modelData.label

                    background: Rectangle {
                        radius: 4
                        color: stateButton.checked ? LibraryStyle.tone("#3A3A3A") :
                                                     stateButton.hovered ? LibraryStyle.hover :
                                                                           "transparent"
                        border.width: stateButton.visualFocus ? 1 : 0
                        border.color: LibraryStyle.focus
                    }
                    contentItem: Item {
                        Text {
                            visible: !!stateButton.modelData.text
                            anchors.centerIn: parent
                            text: stateButton.modelData.text || ""
                            color: stateButton.checked ? LibraryStyle.text : LibraryStyle.muted
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 12
                        }

                        StudioIcon {
                            visible: !!stateButton.modelData.path
                            anchors.centerIn: parent
                            width: 15
                            height: 15
                            tone: stateButton.checked ? LibraryStyle.text : LibraryStyle.muted
                            source: stateButton.modelData.path ? "data:image/svg+xml,"
                                                                 + encodeURIComponent(
                                                                     '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="#cccccc" stroke-width="1.5" stroke-linecap="round"><path d="'
                                                                     + stateButton.modelData.path
                                                                     + '"/></svg>') : ""
                        }
                    }

                    onClicked: group.controller.update(group.propertyName, modelData.value)
                }
            }
        }
    }
    component TypeField: ColumnLayout {
        id: typeField

        required property string label
        property string caption: ""
        property bool unitSelectable: false
        property var value: 0

        signal committed(real amount)
        signal unitClicked

        Layout.fillWidth: true
        Layout.preferredWidth: 1
        spacing: 4

        LibraryCaption {
            text: typeField.label
            font.pixelSize: 10
        }

        InspectorField {
            Layout.fillWidth: true
            caption: typeField.caption
            captionInteractive: typeField.unitSelectable
            value: typeField.value

            onCommitted: value => typeField.committed(Number(value))
            onCaptionClicked: typeField.unitClicked()
        }
    }
}
