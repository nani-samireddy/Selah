pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

ItemDelegate {
    id: card

    required property var record
    property bool lastOpened: false
    readonly property bool engaged: hovered || visualFocus
    readonly property var serviceDate: record.date ? new Date(record.date + "T12:00:00") : null
    readonly property bool dated: serviceDate !== null && !isNaN(serviceDate.getTime())

    padding: 16
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.Button
    Accessible.name: "Open " + record.title + (record.date ? ", " + record.date : "")
    ToolTip.visible: hovered
    ToolTip.text: record.title
    ToolTip.delay: 700

    background: SquircleSurface {
        radius: 12
        border.color: card.visualFocus ? LibraryStyle.focus : card.engaged ? LibraryStyle.tone(
                                                                                 "#606060") :
                                                                             LibraryStyle.border

        fillGradient: LinearGradient {
            x1: 0
            y1: 0
            x2: 0
            y2: card.height

            GradientStop {
                position: 0
                color: card.down ? LibraryStyle.hover : card.engaged ? LibraryStyle.hover :
                                                                       LibraryStyle.field
            }

            GradientStop {
                position: 1
                color: card.down ? LibraryStyle.field : LibraryStyle.panel
            }
        }
    }
    contentItem: ColumnLayout {
        enabled: false
        spacing: 8

        RowLayout {
            Layout.minimumHeight: 20
            Layout.preferredHeight: 20
            Layout.maximumHeight: 20
            spacing: 6

            StudioIcon {
                name: "list-ordered"
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                tone: LibraryStyle.muted
            }

            Text {
                text: "Service program"
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 10
                color: LibraryStyle.muted
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                visible: card.lastOpened
                implicitWidth: recentLabel.implicitWidth + 14
                implicitHeight: 20
                radius: 10
                color: LibraryStyle.hover

                Text {
                    id: recentLabel

                    anchors.centerIn: parent
                    text: "Last opened"
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 9
                    color: LibraryStyle.text
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                implicitWidth: 46
                implicitHeight: 52
                radius: 8
                color: LibraryStyle.tone("#2B2B2B")
                border.color: LibraryStyle.tone("#3B3B3B")

                Column {
                    anchors.centerIn: parent
                    spacing: 1

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: card.dated ? Qt.formatDate(card.serviceDate, "MMM").toLocaleUpperCase(
                                               ) : "DATE"
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 9
                        font.letterSpacing: 0.8
                        color: LibraryStyle.muted
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: card.dated ? Qt.formatDate(card.serviceDate, "dd") : "—"
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 22
                        font.weight: Font.Medium
                        color: LibraryStyle.text
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    Layout.fillWidth: true
                    text: card.record.title
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: LibraryStyle.text
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: card.dated ? Qt.formatDate(card.serviceDate, "dddd · yyyy") :
                                       "No service date"
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 10
                    color: LibraryStyle.muted
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: LibraryStyle.divider
        }

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "Open program"
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 11
                color: card.engaged ? LibraryStyle.text : LibraryStyle.muted
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                implicitWidth: 24
                implicitHeight: 24
                radius: 12
                color: card.engaged ? LibraryStyle.text : LibraryStyle.hover

                StudioIcon {
                    anchors.centerIn: parent
                    name: "chevron-right"
                    width: 12
                    height: 12
                    tone: card.engaged ? LibraryStyle.panel : LibraryStyle.muted
                }
            }
        }
    }

    Accessible.onPressAction: clicked()
}
