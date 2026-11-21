pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    implicitHeight: LibraryStyle.controlHeight
    implicitWidth: 180
    leftPadding: 10
    rightPadding: 30
    topPadding: 0
    bottomPadding: 0
    font.family: LibraryStyle.fontFamily
    font.weight: Font.Normal
    font.pixelSize: 12
    hoverEnabled: true

    background: Rectangle {
        color: control.hovered ? LibraryStyle.hover : LibraryStyle.field
        border.color: control.visualFocus || control.popup.visible ? LibraryStyle.focus :
                                                                     LibraryStyle.border
        radius: LibraryStyle.controlRadius
        opacity: control.enabled ? 1 : 0.5
    }
    contentItem: Text {
        renderType: Text.QtRendering
        text: control.displayText
        font: control.font
        color: control.enabled ? LibraryStyle.text : LibraryStyle.tone("#686868")
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: StudioIcon {
        name: "chevron-down"
        rotation: control.popup.visible ? 180 : 0
        width: 14
        height: 14
        x: control.width - width - 10
        y: (control.height - height) / 2
        tone: control.enabled ? LibraryStyle.tone("#CCCCCC") : LibraryStyle.tone("#555555")

        Behavior on rotation {
            NumberAnimation {
                duration: InteractionFeedback.duration
                easing.type: Easing.OutCubic
            }
        }
    }
    delegate: ItemDelegate {
        id: option

        required property int index

        width: ListView.view.width
        height: 32
        leftPadding: 10
        rightPadding: 24
        // Resolve through ComboBox so both string models and textRole models display labels.
        text: control.textAt(index)
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            renderType: Text.QtRendering
            text: option.text
            font: control.font
            color: LibraryStyle.text
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 4
            color: option.highlighted || option.hovered ? LibraryStyle.tone("#303030") :
                                                          "transparent"

            Rectangle {
                visible: option.index === control.currentIndex
                width: 4
                height: 4
                radius: 2
                color: LibraryStyle.tone("#BBBBBB")
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: 4
        margins: 8
        implicitHeight: Math.min(288, options.contentHeight + 8)

        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: InteractionFeedback.duration
                }

                NumberAnimation {
                    property: "scale"
                    from: 0.98
                    to: 1
                    duration: InteractionFeedback.duration
                    easing.type: Easing.OutCubic
                }
            }
        }
        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: InteractionFeedback.reducedMotion ? 0 : 80
            }
        }
        background: Rectangle {
            color: LibraryStyle.tone("#202020")
            border.color: LibraryStyle.tone("#414141")
            radius: 8
        }
        contentItem: ListView {
            id: options

            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds

            ScrollIndicator.vertical: ScrollIndicator {
            }
        }
    }

    Connections {
        function onActivated() {
            InteractionFeedback.play("select");
        }

        target: control
    }
}
