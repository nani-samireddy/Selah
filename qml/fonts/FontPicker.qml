pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    required property var catalog
    property string previewText: ""
    property string currentFamily: "Arial"
    property string selectedFamily: ""
    property string targetId: ""
    property string scope: "google"
    property string script: "all"
    property string pendingFamily: ""
    property var results: []
    readonly property bool ready: {
        const row = results.find(record => record.family === selectedFamily);
        return row ? row.available : catalog.available(selectedFamily);
    }
    readonly property bool previewReady: catalog.previewFamily === selectedFamily
                                         && catalog.previewAvailable

    signal chosen(string family, string elementId)

    function refresh() {
        results = catalog.search(search.text, scope, script);
    }

    width: Math.min(900, parent.width - 48)
    height: Math.min(680, parent.height - 48)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 24
    modal: true
    focus: true

    background: Rectangle {
        color: LibraryStyle.panel
        radius: 12
        border.color: LibraryStyle.tone("#3C3C3C")
    }

    onSelectedFamilyChanged: if (opened) {
                                 catalog.cancelPreview();
                                 previewDelay.restart();
                             }
    onOpened: {
        selectedFamily = currentFamily;
        search.clear();
        refresh();
        previewDelay.restart();
        search.forceActiveFocus();
    }
    onClosed: {
        previewDelay.stop();
        catalog.cancelPreview();
        if (pendingFamily && catalog.busy && catalog.activeFamily === pendingFamily)
            catalog.cancel();
        pendingFamily = "";
    }

    Timer {
        id: previewDelay

        interval: 250

        onTriggered: if (panel.opened)
                         panel.catalog.requestPreview(panel.selectedFamily)
    }

    Connections {
        function onChanged() {
            panel.refresh();
        }

        function onInstallFailed(message) {
            panel.pendingFamily = "";
        }

        function onFamilyReady(requested, resolved) {
            if (panel.opened && panel.pendingFamily === requested) {
                panel.pendingFamily = "";
                panel.chosen(resolved, panel.targetId);
                panel.close();
            }
        }

        target: panel.catalog
    }

    Dialog {
        id: license

        title: "Font license"
        modal: true
        anchors.centerIn: parent
        width: Math.min(620, panel.width - 40)
        height: Math.min(460, panel.height - 40)
        standardButtons: Dialog.Close

        background: Rectangle {
            color: LibraryStyle.panel
            radius: 10
            border.color: LibraryStyle.border
        }
        contentItem: ScrollView {
            clip: true

            LibraryTextArea {
                readOnly: true
                text: panel.catalog.licenseText(panel.selectedFamily)
                font.pixelSize: 12
                textFormat: TextEdit.PlainText
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Column {
                spacing: 4

                Text {
                    text: "Choose a font"
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 22
                    color: LibraryStyle.text
                }

                LibraryCaption {
                    text: panel.catalog.count.toLocaleString()
                          + " Google Fonts families · Downloads stay available offline"
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: "Close"
                quiet: true

                onClicked: panel.close()
            }
        }

        LibraryField {
            id: search

            Layout.fillWidth: true
            leadingIcon: "search"
            placeholderText: "Search font families…"

            onTextChanged: panel.refresh()
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryTabs {
                options: [
                    {
                        text: "Google Fonts",
                        value: "google"
                    },
                    {
                        text: "Offline",
                        value: "offline"
                    },
                    {
                        text: "System",
                        value: "system"
                    }
                ]
                currentValue: panel.scope

                onChosen: value => {
                    panel.scope = value;
                    panel.refresh();
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryCombo {
                Layout.preferredWidth: 180
                model: [
                    {
                        label: "All languages",
                        value: "all"
                    },
                    {
                        label: "Telugu",
                        value: "telugu"
                    },
                    {
                        label: "Devanagari",
                        value: "devanagari"
                    },
                    {
                        label: "Tamil",
                        value: "tamil"
                    },
                    {
                        label: "Kannada",
                        value: "kannada"
                    },
                    {
                        label: "Malayalam",
                        value: "malayalam"
                    },
                    {
                        label: "Bengali",
                        value: "bengali"
                    },
                    {
                        label: "Gujarati",
                        value: "gujarati"
                    },
                    {
                        label: "Gurmukhi",
                        value: "gurmukhi"
                    },
                    {
                        label: "Odia",
                        value: "oriya"
                    },
                    {
                        label: "Latin / English",
                        value: "latin"
                    }
                ]
                textRole: "label"
                enabled: panel.scope !== "system"

                onActivated: index => {
                    panel.script = model[index].value;
                    panel.refresh();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 24

            ListView {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                model: panel.results
                clip: true
                spacing: 4

                ScrollBar.vertical: ScrollBar {
                }
                delegate: ItemDelegate {
                    id: familyRow

                    required property var modelData

                    width: ListView.view.width - 8
                    height: 56
                    padding: 10
                    Accessible.name: modelData.family

                    background: Rectangle {
                        radius: 6
                        color: panel.selectedFamily === familyRow.modelData.family
                               ? LibraryStyle.tone("#303030") : familyRow.hovered
                                 ? LibraryStyle.tone("#252525") : "transparent"
                    }
                    contentItem: Column {
                        spacing: 5

                        Text {
                            text: familyRow.modelData.family
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 13
                            color: LibraryStyle.text
                            width: parent.width
                            elide: Text.ElideRight
                        }

                        LibraryCaption {
                            text: familyRow.modelData.available ? "Available offline" :
                                                                  familyRow.modelData.category
                                                                  + " · Download"
                        }
                    }

                    onClicked: panel.selectedFamily = modelData.family
                }
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: LibraryStyle.divider
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 16

                Text {
                    text: panel.selectedFamily
                    color: LibraryStyle.text
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 20
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                ScrollView {
                    id: previewScroll

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: availableWidth
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    Column {
                        width: previewScroll.availableWidth

                        Text {
                            width: parent.width
                            objectName: "fontSelectionPreview"
                            visible: panel.ready
                            textFormat: Text.PlainText
                            text: panel.ready ? (panel.previewText || "Aa Bb Cc\n1234567890") : ""
                            font.family: panel.ready ? panel.catalog.resolvedFamily(
                                                           panel.selectedFamily) :
                                                       LibraryStyle.fontFamily
                            font.pixelSize: 30
                            color: LibraryStyle.tone("#E6E6E6")
                            wrapMode: Text.Wrap
                        }

                        FontPreviewItem {
                            id: rawPreview

                            objectName: "downloadableFontPreview"
                            width: parent.width
                            height: visible ? implicitHeight : 0
                            visible: !panel.ready && panel.previewReady
                            fontBytes: panel.catalog.previewBytes
                            text: panel.previewText || "Aa Bb Cc\n1234567890"
                            color: LibraryStyle.text
                        }

                        BusyIndicator {
                            anchors.horizontalCenter: parent.horizontalCenter
                            running: panel.opened && !panel.ready && !panel.previewReady
                                     && panel.catalog.previewBusy
                            visible: running
                        }
                    }
                }

                LibraryCaption {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: panel.ready ? "Preview uses the selected family with language fallbacks." :
                                        panel.previewReady ? (rawPreview.missingCharacters
                                                              ? "This font does not include some of the selected characters." :
                                                                rawPreview.truncated
                                                                ? "Preview shortened for display." :
                                                                  panel.catalog.previewStatus) :
                                                             panel.catalog.previewFamily
                                                             === panel.selectedFamily
                                                             ? panel.catalog.previewStatus :
                                                               "Loading preview…"
                }

                LibraryButton {
                    text: "Retry preview"
                    quiet: true
                    visible: !panel.ready && !panel.previewReady && !panel.catalog.previewBusy
                             && panel.catalog.previewFamily === panel.selectedFamily

                    onClicked: panel.catalog.requestPreview(panel.selectedFamily)
                }

                LibraryButton {
                    text: "License & attribution"
                    enabled: panel.ready
                    quiet: true

                    onClicked: license.open()
                }
            }
        }

        Text {
            text: panel.catalog.status
            color: LibraryStyle.muted
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryCaption {
                text: panel.results.length + " families"
                Layout.fillWidth: true
            }

            LibraryButton {
                visible: panel.catalog.busy
                text: "Cancel download"

                onClicked: {
                    panel.catalog.cancel();
                    panel.pendingFamily = "";
                }
            }

            LibraryButton {
                text: panel.catalog.busy ? "Downloading · " + panel.catalog.progress + "%" :
                                           panel.ready ? "Use font" : "Download & use"
                accented: true
                enabled: panel.selectedFamily.length > 0 && !panel.catalog.busy

                onClicked: {
                    previewDelay.stop();
                    if (panel.catalog.previewBusy)
                        panel.catalog.cancelPreview();
                    panel.pendingFamily = panel.selectedFamily;
                    panel.catalog.install(panel.selectedFamily);
                }
            }
        }
    }
}
