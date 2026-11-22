pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: progress

    required property var library
    readonly property bool downloading: library.bibleImportPhase === "downloading"
    readonly property real fraction: library.bibleDownloadTotal > 0 ? Math.min(1,
                                                                               library.bibleDownloadedBytes
                                                                               / library.bibleDownloadTotal) :
                                                                      0

    visible: library.busy
    spacing: 6

    RowLayout {
        Text {
            text: progress.downloading ? (progress.library.bibleDownloadedBytes > 0
                                          ? "Downloading Bible…" : "Connecting to GitHub…") :
                                         "Importing and indexing…"
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 11
            color: LibraryStyle.text
            Layout.fillWidth: true
        }

        Text {
            text: !progress.downloading ? "" : (progress.library.bibleDownloadedBytes
                                                / 1048576).toFixed(1) + (
                                              progress.library.bibleDownloadTotal > 0 ? " / " + (
                                                                                            progress.library.bibleDownloadTotal
                                                                                            / 1048576).toFixed(
                                                                                            1) + " MB · "
                                                                                        + Math.floor(
                                                                                            progress.fraction
                                                                                            * 100) + "%" :
                                                                                        " MB")
            font.pixelSize: 10
            color: LibraryStyle.muted
        }
    }

    ProgressBar {
        objectName: "bibleImportProgressBar"
        Layout.fillWidth: true
        from: 0
        to: 1
        value: progress.fraction
        indeterminate: !progress.downloading || progress.library.bibleDownloadTotal <= 0
        palette.highlight: LibraryStyle.dark ? "#AEC8B7" : "#456C55"

        background: Rectangle {
            implicitHeight: 6
            color: LibraryStyle.border
            radius: 3
        }
    }
}
