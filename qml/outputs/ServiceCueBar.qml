pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: bar

    required property var controller
    readonly property bool dialogOpen: settings.opened

    spacing: 6

    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 6

        Text {
            text: "Service cues"
            color: LibraryStyle.muted
            height: 30
            verticalAlignment: Text.AlignVCenter
            rightPadding: 8
        }

        Repeater {
            model: bar.controller.cues

            LibraryButton {
                required property var modelData
                required property int index

                text: modelData.name
                enabled: !bar.controller.running

                onClicked: bar.controller.trigger(index)
            }
        }

        LibraryButton {
            visible: bar.controller.running
            text: "Cancel cue"

            onClicked: bar.controller.cancel()
        }

        LibraryButton {
            iconName: "settings-2"
            hint: "Configure service cues"

            onClicked: settings.open()
        }
    }

    Text {
        text: bar.controller.status
        visible: text.length > 0
        color: LibraryStyle.muted
        font.pixelSize: 11
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    ServiceCueDialog {
        id: settings

        controller: bar.controller
    }
}
