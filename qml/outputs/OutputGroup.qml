import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shared spacing and surface for the output settings sections.
Pane {
    Layout.fillWidth: true
    padding: 16
    implicitHeight: implicitContentHeight + topPadding + bottomPadding

    contentItem: ColumnLayout {
        spacing: 12
    }
    background: Rectangle {
        radius: 10
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
    }
}
