pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: settings

    required property var document

    objectName: "editorTransitions"
    spacing: 6

    LibraryCaption {
        text: "Slide transitions"
        color: LibraryStyle.text
        font.weight: Font.Medium
    }

    LibraryCaption {
        text: "Default for presentation"
    }

    TransitionField {
        Layout.fillWidth: true
        controlName: "presentationTransition"
        transition: settings.document.defaultTransition

        onChosen: (type, durationMs) => settings.document.setDefaultTransition(type, durationMs)
    }

    LibraryCaption {
        text: "This slide"
        Layout.topMargin: 4
    }

    TransitionField {
        Layout.fillWidth: true
        controlName: "slideTransition"
        allowInherit: true
        inheritLabel: "Use default"
        transition: settings.document.slideTransition

        onChosen: (type, durationMs) => settings.document.setSlideTransition(type, durationMs)
    }
}
