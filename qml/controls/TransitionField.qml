pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: field

    property var transition: ({})
    property bool allowInherit: false
    property string inheritLabel: "Default"
    property string controlName: ""
    readonly property var effects: [
        {
            name: "Cut",
            type: "cut"
        },
        {
            name: "Fade",
            type: "fade"
        },
        {
            name: "Fade through black",
            type: "fade-black"
        },
        {
            name: "Fade through white",
            type: "fade-white"
        },
        {
            name: "Wipe left",
            type: "wipe-left"
        },
        {
            name: "Wipe right",
            type: "wipe-right"
        },
        {
            name: "Wipe up",
            type: "wipe-up"
        },
        {
            name: "Wipe down",
            type: "wipe-down"
        },
        {
            name: "Slide left",
            type: "push-left"
        },
        {
            name: "Slide right",
            type: "push-right"
        },
        {
            name: "Slide up",
            type: "push-up"
        },
        {
            name: "Slide down",
            type: "push-down"
        },
        {
            name: "Swipe left",
            type: "swipe-left"
        },
        {
            name: "Swipe right",
            type: "swipe-right"
        },
        {
            name: "Swipe up",
            type: "swipe-up"
        },
        {
            name: "Swipe down",
            type: "swipe-down"
        },
        {
            name: "Zoom in",
            type: "zoom-in"
        },
        {
            name: "Zoom out",
            type: "zoom-out"
        },
        {
            name: "Spin",
            type: "spin"
        },
        {
            name: "Blur",
            type: "blur"
        },
        {
            name: "Iris in",
            type: "iris-in"
        },
        {
            name: "Iris out",
            type: "iris-out"
        },
        {
            name: "Paint brush",
            type: "brush"
        }
    ]
    readonly property var choices: allowInherit ? [
                                                      {
                                                          name: inheritLabel,
                                                          type: "inherit"
                                                      }
                                                  ].concat(effects) : effects

    signal chosen(string type, int durationMs)

    spacing: 6

    LibraryCombo {
        objectName: field.controlName + "Type"
        Layout.fillWidth: true
        model: field.choices
        textRole: "name"
        currentIndex: Math.max(0, field.choices.findIndex(choice => choice.type === (
                                                                        field.transition.type
                                                                        || "inherit")))
        Accessible.name: "Slide transition"

        onActivated: index => field.chosen(field.choices[index].type, duration.value)
    }

    RowLayout {
        Layout.fillWidth: true
        visible: !!field.transition.type && !["cut", "inherit"].includes(field.transition.type)

        LibraryCaption {
            text: "Duration"
            Layout.fillWidth: true
        }

        LibrarySpinBox {
            id: duration

            objectName: field.controlName + "Duration"
            implicitWidth: 88
            from: 50
            to: 5000
            stepSize: 50
            value: field.transition.durationMs || 300
            Accessible.name: "Transition duration in milliseconds"

            onValueModified: field.chosen(field.transition.type, value)
        }

        Text {
            visible: duration.visible
            text: "ms"
            color: LibraryStyle.muted
            font.pixelSize: 10
        }
    }
}
