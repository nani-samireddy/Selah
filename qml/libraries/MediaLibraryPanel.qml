pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    property bool workspaceMode: false
    property var library
    property var programTarget: null
    property var document
    property string message: ""
    property string kindFilter: "all"
    property var filteredAssets: {
        let query = search.text.toLowerCase();
        return library.assets.filter(asset => {
            return (kindFilter === "all" || asset.kind === kindFilter) && asset.name.toLowerCase(
                        ).includes(query);
        });
    }

    signal contentAdded(bool toProgram)
    signal importRequested

    function addAsset(asset, programId = "") {
        const ok = programTarget ? programTarget.addMedia(asset, programId) : document.useMedia(
                                       asset);
        if (ok && (!programTarget || !programId || programId === programTarget.id))
            finishAddition();
        else
            message = programTarget ? programTarget.status : document.status;
    }

    function finishAddition() {
        let toProgram = !!programTarget;
        close();
        contentAdded(toProgram);
    }

    closePolicy: workspaceMode ? Popup.NoAutoClose : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: workspaceMode ? parent.width : Math.min(880, parent.width - 60)
    height: workspaceMode ? parent.height : Math.min(630, parent.height - 40)
    x: workspaceMode ? 0 : (parent.width - width) / 2
    y: workspaceMode ? 0 : (parent.height - height) / 2
    modal: !workspaceMode
    focus: true
    padding: LibraryStyle.pagePadding

    background: Rectangle {
        color: LibraryStyle.panel
        border.width: panel.workspaceMode ? 0 : 1
        border.color: LibraryStyle.tone("#3B3B3B")
        radius: panel.workspaceMode ? 0 : 12
    }

    onOpened: {
        library.refresh();
        search.forceActiveFocus();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Column {
                spacing: 5

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: "Media library"
                    color: LibraryStyle.tone("#ECECEC")
                    font.pixelSize: 22
                    font.weight: Font.Medium
                }

                Text {
                    font.family: LibraryStyle.fontFamily
                    text: "Import once. Reuse across presentations."
                    color: LibraryStyle.tone("#929292")
                    font.pixelSize: 12
                }
            }

            Item {
                Layout.fillWidth: true
            }

            LibraryButton {
                text: panel.document.importing ? "Importing…" : "Import media"
                iconName: "plus"
                enabled: !panel.document.importing

                onClicked: panel.importRequested()
            }

            LibraryButton {
                text: "Close"
                visible: !panel.workspaceMode
                quiet: true

                onClicked: panel.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true

            LibraryField {
                id: search

                leadingIcon: "search"
                Layout.fillWidth: true
                placeholderText: "Search media…"
            }

            LibraryTabs {
                options: [
                    {
                        text: "All",
                        value: "all"
                    },
                    {
                        text: "Images",
                        value: "image"
                    },
                    {
                        text: "Videos",
                        value: "video"
                    }
                ]
                currentValue: panel.kindFilter

                onChosen: value => panel.kindFilter = value
            }
        }

        Text {
            font.family: LibraryStyle.fontFamily
            visible: panel.message.length > 0 || panel.library.error.length > 0
            text: panel.message || panel.library.error
            color: panel.message ? LibraryStyle.muted : "#EAA5A5"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            font.pixelSize: 12
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                GridLayout {
                    id: assetGrid

                    width: parent.width
                    columns: Math.max(2, Math.min(4, Math.floor(width / 260)))
                    columnSpacing: 16
                    rowSpacing: 18

                    Repeater {
                        model: panel.filteredAssets

                        delegate: ColumnLayout {
                            id: asset

                            required property var modelData

                            Layout.fillWidth: true
                            Layout.preferredWidth: (assetGrid.width - (assetGrid.columns - 1)
                                                    * assetGrid.columnSpacing) / assetGrid.columns
                            Layout.maximumWidth: Layout.preferredWidth
                            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                            spacing: 7

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 118
                                Layout.preferredHeight: width * 9 / 16
                                radius: 7
                                color: LibraryStyle.tone("#242424")
                                border.color: LibraryStyle.tone("#383838")
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    source: asset.modelData.thumbnail
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                }

                                StudioIcon {
                                    visible: !asset.modelData.thumbnail
                                    anchors.centerIn: parent
                                    name: asset.modelData.kind === "video" ? "play" : "image"
                                    width: 24
                                    height: 24
                                }

                                Rectangle {
                                    width: tag.implicitWidth + 12
                                    height: 21
                                    color: "#D0181818"
                                    radius: 4

                                    anchors {
                                        left: parent.left
                                        bottom: parent.bottom
                                        margins: 7
                                    }

                                    Text {
                                        id: tag

                                        font.family: LibraryStyle.fontFamily
                                        anchors.centerIn: parent
                                        text: !asset.modelData.available ? "Missing" :
                                                                           asset.modelData.kind
                                                                           === "video" ? "Video" :
                                                                                         "Image"
                                        font.pixelSize: 10
                                        color: LibraryStyle.tone("#DDDDDD")
                                    }
                                }
                            }

                            Text {
                                font.family: LibraryStyle.fontFamily
                                text: asset.modelData.name
                                color: LibraryStyle.tone("#D8D8D8")
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    font.family: LibraryStyle.fontFamily
                                    text: (asset.modelData.bytes / 1048576).toFixed(1) + " MB"
                                    color: LibraryStyle.tone("#777777")
                                    font.pixelSize: 10
                                    Layout.fillWidth: true
                                }

                                LibraryButton {
                                    visible: !panel.programTarget
                                    text: "Add to slide"
                                    implicitHeight: 28
                                    enabled: asset.modelData.available

                                    onClicked: panel.addAsset(asset.modelData)
                                }
                            }

                            ProgramAddButton {
                                objectName: "mediaProgramDestination_" + asset.modelData.source
                                Layout.fillWidth: true
                                visible: !!panel.programTarget
                                store: panel.programTarget
                                ready: asset.modelData.available

                                onAddRequested: programId => panel.addAsset(asset.modelData,
                                                                            programId)
                            }
                        }
                    }
                }
            }

            LibraryEmptyState {
                anchors.centerIn: parent
                width: Math.min(320, parent.width - 48)
                visible: panel.filteredAssets.length === 0
                title: search.text.length ? "No matching media" : "Your media starts here"
                description: search.text.length ? "Try another name or change the media filter." :
                                                  "Import images and videos to reuse across songs, slides, and service programs."
                iconName: search.text.length ? "search" : "image"
            }
        }

        Text {
            font.family: LibraryStyle.fontFamily
            text: panel.library.assets.length + " assets · Stored on this computer"
            color: LibraryStyle.tone("#777777")
            font.pixelSize: 10
        }
    }
}
