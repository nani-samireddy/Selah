pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ComboBox {
    id: box

    implicitHeight: 32
    font.pixelSize: 12

    background: Rectangle {
        radius: 5
        color: box.hovered ? "#2D2D2D" : "#252525"
        border.color: box.activeFocus ? "#888888" : "#383838"
    }
    contentItem: Text {
        leftPadding: 10
        rightPadding: 26
        text: box.displayText
        color: "#D6D6D6"
        font: box.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: StudioIcon {
        name: "chevron-down"
        x: box.width - 24
        y: (box.height - height) / 2
        tone: "#8B8B8B"
    }
    delegate: ItemDelegate {
        id: optionDelegate

        required property var modelData

        width: box.width
        height: 32

        contentItem: Text {
            text: optionDelegate.modelData
            color: "#DEDEDE"
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: optionDelegate.hovered ? "#383838" : "#242424"
        }
    }
    popup: Popup {
        y: box.height + 5
        width: box.width
        padding: 4
        implicitHeight: Math.min(240, contentItem.implicitHeight + 8)

        background: Rectangle {
            color: "#242424"
            radius: 7
            border.color: "#444444"
        }
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: box.popup.visible ? box.delegateModel : null
            currentIndex: box.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator {
            }
        }
    }
}
