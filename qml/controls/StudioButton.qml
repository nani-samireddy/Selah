import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool accented: false
    property bool quiet: false
    property string hint: ""
    property string iconName: ""

    padding: 0
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    implicitHeight: 34
    implicitWidth: Math.max(34, label.implicitWidth + (iconName.length ? 23 : 0) + 22)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    ToolTip.visible: hovered && hint.length > 0
    ToolTip.text: hint
    ToolTip.delay: 500
    Accessible.name: hint.length ? hint : text

    contentItem: Item {
        Row {
            anchors.centerIn: parent
            spacing: 7

            StudioIcon {
                visible: control.iconName.length > 0
                width: 14
                height: 14
                name: control.iconName
                tone: control.accented ? "#171717" : control.enabled ? "#B8B8B8" : "#606060"
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: label

                text: control.text
                visible: text.length > 0
                color: !control.enabled ? "#606060" : control.accented ? "#171717" :
                                                                         control.checked
                                                                         ? "#F1F1F1" : "#CBCBCB"
                font.family: "Arial"
                font.pixelSize: 12
                font.weight: control.accented ? Font.DemiBold : Font.Normal
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
    background: Rectangle {
        radius: 6
        color: !control.enabled ? (control.quiet ? "transparent" : "#232323") : control.accented ? (control.down
                                                                                                    ? "#BDBDBD" :
                                                                                                      control.hovered
                                                                                                      ? "#FFFFFF" :
                                                                                                        "#E5E5E5") :
                                                                                                   control.checked
                                                                                                   ? "#3A3A3A" :
                                                                                                     control.down
                                                                                                     ? "#383838" :
                                                                                                       control.hovered
                                                                                                       ? "#333333" :
                                                                                                         control.quiet
                                                                                                         ? "transparent" :
                                                                                                           "#292929"
        border.width: control.visualFocus ? 1 : control.quiet || control.accented ? 0 : 1
        border.color: control.visualFocus ? "#AAAAAA" : "#414141"

        Behavior on color {
            ColorAnimation {
                duration: 110
            }
        }
    }
}
