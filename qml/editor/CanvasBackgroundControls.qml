pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: settings

    required property var host
    readonly property color fill: host.doc.backgroundColor
    readonly property bool transparent: fill.a === 0
    readonly property string mode: transparent ? "transparent" : host.doc.backgroundFill.type
                                                 || "color"

    function chooseMode(value) {
        if (value === "transparent") {
            host.doc.setBackgroundTransparent(true);
            return;
        }
        if (value === "image") {
            imagePicker.open();
            return;
        }
        host.doc.setBackgroundTransparent(false);
        if (["linear", "radial"].includes(value)) {
            const previous = host.doc.backgroundFill;
            const stops = previous.stops || [
                      {
                          color: "#213547",
                          position: 0
                      },
                      {
                          color: "#86749C",
                          position: 1
                      }
                  ];
            host.doc.setBackgroundFill({
                                           type: value,
                                           angle: previous.angle || 0,
                                           stops: stops
                                       });
            host.doc.setAmbientOpacity(0);
        } else
            host.doc.setBackgroundFill({
                                           type: "color"
                                       });
    }

    objectName: "canvasBackgroundControls"
    spacing: 6

    LibraryCaption {
        text: "Background"
        color: LibraryStyle.text
        font.weight: Font.Medium
    }

    LibraryCombo {
        signal chosen(string value)

        objectName: "canvasBackgroundMode"
        Layout.fillWidth: true
        model: [
            {
                name: "Solid color",
                value: "color"
            },
            {
                name: "Linear gradient",
                value: "linear"
            },
            {
                name: "Radial gradient",
                value: "radial"
            },
            {
                name: "Image",
                value: "image"
            },
            {
                name: "Transparent",
                value: "transparent"
            }
        ]
        textRole: "name"
        currentIndex: Math.max(0, model.findIndex(option => option.value === settings.mode))

        onChosen: value => settings.chooseMode(value)
        onActivated: index => chosen(model[index].value)
    }

    GradientFillControls {
        visible: ["linear", "radial"].includes(settings.mode)
        Layout.fillWidth: true
        document: settings.host.doc
    }

    ColumnLayout {
        visible: settings.mode === "image"
        Layout.fillWidth: true

        Image {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            source: settings.host.doc.backgroundFill.source || ""
            sourceSize.width: 480
            sourceSize.height: 0
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }

        LibraryButton {
            text: settings.host.doc.importing ? "Importing image…" : "Replace image"
            iconName: "image"
            Layout.fillWidth: true
            enabled: !settings.host.doc.importing

            onClicked: imagePicker.open()
        }

        LibraryCombo {
            Layout.fillWidth: true
            model: [
                {
                    name: "Fill · crop to canvas",
                    value: "cover"
                },
                {
                    name: "Fit · show entire image",
                    value: "contain"
                },
                {
                    name: "Stretch",
                    value: "stretch"
                }
            ]
            textRole: "name"
            currentIndex: Math.max(0, model.findIndex(option => option.value
                                                                === settings.host.doc.backgroundFill.fit))

            onActivated: index => settings.host.doc.setBackgroundFill(Object.assign({},
                                                                                    settings.host.doc.backgroundFill,
                                                                                    {
                                                                                        fit: model[index].value
                                                                                    }))
        }
    }

    FileDialog {
        id: imagePicker

        title: "Choose a canvas background"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.avif *.heic *.bmp *.svg *.tif *.tiff)"]

        onAccepted: settings.host.doc.importBackground(selectedFile)
    }

    ColorField {
        objectName: "canvasBackgroundColor"
        visible: ["color", "image"].includes(settings.mode)
        Layout.fillWidth: true
        label: "Canvas background color"
        value: settings.host.doc.backgroundColor

        onPickerRequested: settings.host.showBackgroundPicker()
        onCommitted: value => settings.host.doc.setBackground(value)
    }

    RowLayout {
        visible: !settings.transparent
        Layout.fillWidth: true

        LibraryCaption {
            text: "Fill opacity"
            Layout.fillWidth: true
        }

        LibrarySpinBox {
            objectName: "canvasFillOpacity"
            from: 0
            to: 100
            value: Math.round(settings.fill.a * 100)
            Accessible.name: "Canvas fill opacity percent"

            onValueModified: settings.host.doc.setBackground(Qt.rgba(settings.fill.r, settings.fill.g,
                                                                     settings.fill.b, value
                                                                     / 100).toString())
        }

        LibraryCaption {
            text: "%"
        }
    }

    LibraryButton {
        objectName: "canvasAmbientToggle"
        visible: !settings.transparent
        text: "Ambient pattern"
        selected: settings.host.doc.ambientOpacity > 0
        quiet: !selected
        Layout.fillWidth: true
        hint: "Toggle the optional ambient background pattern"

        onClicked: settings.host.doc.setAmbientOpacity(settings.host.doc.ambientOpacity > 0 ? 0 :
                                                                                              0.5)
    }

    LibraryCaption {
        visible: settings.transparent
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: "Uncovered canvas areas are transparent."
    }
}
