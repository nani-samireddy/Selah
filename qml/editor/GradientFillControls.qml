pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: controls

    required property var document
    readonly property var fill: document.backgroundFill
    readonly property var stops: fill.stops || []

    function update(patch) {
        document.setBackgroundFill(Object.assign({}, fill, patch));
    }

    function updateStop(index, patch) {
        let next = stops.map(stop => Object.assign({}, stop));
        next[index] = Object.assign(next[index], patch);
        next.sort((a, b) => a.position - b.position);
        update({
                   stops: next
               });
    }

    spacing: 6

    RowLayout {
        visible: controls.fill.type === "linear"

        LibraryCaption {
            text: "Angle"
            Layout.fillWidth: true
        }

        LibrarySpinBox {
            objectName: "canvasGradientAngle"
            from: 0
            to: 360
            value: controls.fill.angle || 0
            Accessible.name: "Gradient angle in degrees"

            onValueModified: controls.update({
                                                 angle: value
                                             })
        }

        LibraryCaption {
            text: "°"
        }
    }

    LibraryCaption {
        text: "Color stops · position %"
    }

    Repeater {
        model: controls.stops

        RowLayout {
            id: row

            required property var modelData
            required property int index

            Layout.fillWidth: true
            spacing: 4

            ColorDialog {
                id: stopPicker

                title: "Gradient stop color"
                options: ColorDialog.ShowAlphaChannel
                selectedColor: row.modelData.color

                onAccepted: controls.updateStop(row.index, {
                                                    color: selectedColor.toString()
                                                })
            }

            ColorField {
                Layout.fillWidth: true
                label: "Gradient stop color"
                value: row.modelData.color

                onPickerRequested: stopPicker.open()
                onCommitted: value => controls.updateStop(row.index, {
                                                              color: value
                                                          })
            }

            LibrarySpinBox {
                implicitWidth: 72
                from: 0
                to: 100
                value: Math.round(row.modelData.position * 100)
                Accessible.name: "Gradient stop position"

                onValueModified: controls.updateStop(row.index, {
                                                         position: value / 100
                                                     })
            }

            LibraryButton {
                iconName: "trash-2"
                implicitWidth: 26
                enabled: controls.stops.length > 2
                hint: "Remove gradient stop"

                onClicked: controls.update({
                                               stops: controls.stops.filter((stop, i) => i
                                                                                         !== row.index)
                                           })
            }
        }
    }

    LibraryButton {
        text: "Add color stop"
        iconName: "plus"
        enabled: controls.stops.length < 8

        onClicked: {
            let next = controls.stops.slice();
            next.push({
                          color: "#FFFFFF",
                          position: 0.5
                      });
            next.sort((a, b) => a.position - b.position);
            controls.update({
                                stops: next
                            });
        }
    }
}
