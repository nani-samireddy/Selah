import QtQuick

StudioButton {
    id: button

    property bool selectionBackground: true

    labelFontFamily: LibraryStyle.fontFamily
    labelFontSize: 13
    implicitHeight: LibraryStyle.controlHeight

    background: Rectangle {
        radius: LibraryStyle.controlRadius
        opacity: button.enabled ? 1 : 0.45
        color: {
            if (button.accented)
                return button.down ? LibraryStyle.tone("#C7C7C7") : button.hovered
                                     ? LibraryStyle.tone("#FFFFFF") : LibraryStyle.tone("#E8E8E8");
            if (button.selectionBackground && (button.selected || button.checked))
                return LibraryStyle.tone("#303030");
            if (button.down)
                return LibraryStyle.tone("#303030");
            if (button.hovered)
                return LibraryStyle.tone("#292929");
            return button.quiet ? "transparent" : LibraryStyle.tone("#1F1F1F");
        }
        border.width: (button.quiet || !button.selectionBackground) && !button.visualFocus ? 0 : 1
        border.color: button.visualFocus ? LibraryStyle.focus : button.accented ? color : button.hovered
                                                                                  ? LibraryStyle.tone(
                                                                                        "#505050") :
                                                                                    LibraryStyle.border

        Behavior on color {
            ColorAnimation {
                duration: InteractionFeedback.duration
            }
        }
    }
}
