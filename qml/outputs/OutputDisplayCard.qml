pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

OutputGroup {
    id: card

    required property var controller
    required property string role
    property string pendingDisplayId: ""
    readonly property bool active: role === "audience" ? controller.audienceEnabled :
                                                         controller.stageEnabled
    readonly property bool black: role === "audience" ? controller.audienceBlack :
                                                        controller.stageBlack
    readonly property string assignedId: role === "audience" ? controller.audienceDisplay :
                                                               controller.stageDisplay
    readonly property var selectedDisplay: controller.displays[displayChoice.currentIndex] || ({})

    objectName: role + "DisplayCard"
    Layout.preferredWidth: 1
    Layout.minimumWidth: 0

    RowLayout {
        Layout.fillWidth: true

        Text {
            text: card.role === "audience" ? "Audience" : "Stage / speaker"
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 15
            font.weight: Font.Medium
            color: LibraryStyle.text
            Layout.fillWidth: true
        }

        Rectangle {
            implicitWidth: stateLabel.implicitWidth + 16
            implicitHeight: 23
            radius: 6
            color: card.active ? (card.black ? LibraryStyle.tone("#3D3328") : LibraryStyle.tone(
                                                   "#25372D")) : LibraryStyle.field

            Text {
                id: stateLabel

                anchors.centerIn: parent
                text: !card.active ? "Stopped" : card.black ? "Blackout" : "Active"
                color: card.active ? (card.black ? (LibraryStyle.dark ? "#C79C67" : "#704819") : (
                                                       LibraryStyle.dark ? "#A7CFB2" : "#346F48")) :
                                     LibraryStyle.muted
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 11
            }
        }
    }

    Text {
        text: card.role === "audience" ? "Slides and media for the congregation" :
                                         "Current slide, next slide, notes and timers"
        font.family: LibraryStyle.fontFamily
        font.pixelSize: 12
        color: LibraryStyle.muted
        Layout.fillWidth: true
        wrapMode: Text.Wrap
    }

    LibraryCaption {
        text: "Fullscreen display"
    }

    LibraryCombo {
        id: displayChoice

        objectName: card.role + "DisplayChoice"
        Layout.fillWidth: true
        model: card.controller.displays
        textRole: "name"
        valueRole: "id"
        Accessible.name: card.role === "audience" ? "Audience fullscreen display" :
                                                    "Speaker fullscreen display"
        currentIndex: {
            let index = model.findIndex(display => display.id === (card.pendingDisplayId
                                                                   || card.assignedId));
            if (index >= 0)
                return index;
            index = model.findIndex(display => !display.operator);
            return index >= 0 ? index : model.length > 0 ? 0 : -1;
        }
        ToolTip.visible: hovered && currentIndex >= 0
        ToolTip.text: card.selectedDisplay.name || ""
        ToolTip.delay: 700

        onActivated: card.pendingDisplayId = currentValue
    }

    Text {
        text: displayChoice.currentIndex < 0 ? "No display available" :
                                               card.selectedDisplay.operator
                                               ? "Operator screen · use Preview" :
                                                 "Available for fullscreen output"
        font.family: LibraryStyle.fontFamily
        font.pixelSize: 11
        color: LibraryStyle.muted
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    RowLayout {
        Layout.fillWidth: true

        LibraryButton {
            objectName: card.role + "FullscreenButton"
            text: "Fullscreen"
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            enabled: displayChoice.currentIndex >= 0 && !card.selectedDisplay.operator
            hint: card.selectedDisplay.operator ? "Choose an external display for fullscreen" :
                                                  "Show on the selected display"

            onClicked: {
                if (card.controller.route(card.role, displayChoice.currentValue))
                    card.pendingDisplayId = "";
            }
        }

        LibraryButton {
            objectName: card.role + "PreviewButton"
            text: "Preview"
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            enabled: card.controller.displays.length > 0
            hint: "Open a windowed preview on the operator screen"

            onClicked: card.controller.preview(card.role)
        }
    }

    RowLayout {
        Layout.fillWidth: true

        LibraryButton {
            objectName: card.role + "BlackoutButton"
            text: card.black ? "Restore" : "Blackout"
            checked: card.black
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            onClicked: card.controller.blackout(card.role, !card.black)
        }

        LibraryButton {
            objectName: card.role + "StopButton"
            text: "Stop output"
            enabled: card.active
            Layout.fillWidth: true
            Layout.preferredWidth: 1

            onClicked: card.controller.stop(card.role)
        }
    }
}
