pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

// Reflects document layer order and delegates every persistent edit to the document model.
StudioPanel {
    id: layersPanel

    required property var host

    x: 18
    y: 20
    width: 218
    height: Math.min(520, layersContent.implicitHeight + 18)

    ColumnLayout {
        id: layersContent

        spacing: 8

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 10
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 3

            Text {
                text: "Layers"
                color: "#E4E4E4"
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            StudioButton {
                iconName: "plus"
                implicitHeight: 26
                implicitWidth: 26
                quiet: true
                hint: "Add text layer"

                onClicked: layersPanel.host.doc.addText()
            }
        }

        StudioRule {
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 5
            Layout.bottomMargin: 5

            StudioIcon {
                name: "chevron-down"
                tone: "#7B7B7B"
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
            }

            StudioIcon {
                name: "layers"
                tone: "#9E9E9E"
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
            }

            Text {
                text: layersPanel.host.doc.slides[layersPanel.host.doc.currentSlide].name
                elide: Text.ElideRight
                color: "#C1C1C1"
                font.pixelSize: 11
                Layout.fillWidth: true
            }

            StudioMicroLabel {
                text: "16:9"
                font.pixelSize: 9
                font.letterSpacing: 0
            }
        }

        Repeater {
            model: layersPanel.host.doc.elements

            delegate: Rectangle {
                id: layerRow

                required property var modelData
                required property int index

                Layout.fillWidth: true
                implicitHeight: 36
                radius: 5
                color: layersPanel.host.doc.selected === index ? "#343434" :
                                                                 layerMouse.containsMouse
                                                                 ? "#242424" : "transparent"
                border.width: layersPanel.host.doc.selected === index ? 1 : 0
                border.color: "#494949"

                MouseArea {
                    id: layerMouse

                    anchors.fill: parent
                    hoverEnabled: true

                    onClicked: layersPanel.host.doc.selected = layerRow.index
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 4
                    spacing: 8

                    StudioIcon {
                        name: layerRow.modelData.kind === "image" ? "image" :
                                                                    layerRow.modelData.kind
                                                                    === "video" ? "play" : "type"
                        tone: "#909090"
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                    }

                    Text {
                        text: layerRow.modelData.name
                        color: layersPanel.host.doc.selected === layerRow.index ? "#F1F1F1" :
                                                                                  "#ADADAD"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    StudioButton {
                        iconName: layerRow.modelData.visible ? "eye" : "eye-off"
                        quiet: true
                        implicitWidth: 23
                        implicitHeight: 25
                        hint: "Toggle layer visibility"

                        onClicked: layersPanel.host.doc.updateElement(layerRow.index, {
                                                                          visible:
                                                                              !layerRow.modelData.visible
                                                                      })
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 35
            color: "transparent"

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    layersPanel.host.doc.selected = -1;
                    layersPanel.host.showBackgroundPicker();
                }
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                x: 12
                spacing: 10

                StudioIcon {
                    name: "image"
                    tone: "#909090"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "Ambient background"
                    color: "#888888"
                    font.pixelSize: 11
                }
            }
        }

        StudioRule {
        }

        RowLayout {
            Layout.fillWidth: true

            StudioButton {
                text: "Lower"
                quiet: true
                enabled: layersPanel.host.doc.selected > 0
                Layout.fillWidth: true
                hint: "Move selected layer backward"

                onClicked: layersPanel.host.doc.moveSelected(-1)
            }

            StudioButton {
                text: "Raise"
                quiet: true
                enabled: layersPanel.host.doc.selected >= 0 && layersPanel.host.doc.selected
                         < layersPanel.host.doc.elements.length - 1
                Layout.fillWidth: true
                hint: "Move selected layer forward"

                onClicked: layersPanel.host.doc.moveSelected(1)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            StudioButton {
                text: "Duplicate"
                quiet: true
                enabled: layersPanel.host.doc.selected >= 0
                Layout.fillWidth: true

                onClicked: layersPanel.host.doc.duplicateSelected()
            }

            StudioButton {
                text: "Delete"
                quiet: true
                enabled: layersPanel.host.doc.selected >= 0
                Layout.fillWidth: true

                onClicked: layersPanel.host.doc.deleteSelected()
            }
        }
    }
}
