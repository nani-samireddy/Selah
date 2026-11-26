pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

SquircleSurface {
    id: card

    required property var controller
    required property var payload
    required property int index
    required property int actionCount
    readonly property string actionType: payload.type || ""
    readonly property var schema: controller.actionTypes.find(row => row.type === actionType) || ({
                                                                                                      label: actionType,
                                                                                                      fields: []
                                                                                                  })

    signal edited(string key, var value)
    signal moveRequested(int delta)
    signal removeRequested
    signal browseRequested(string kind)

    color: LibraryStyle.field
    border.color: LibraryStyle.border
    radius: 10
    implicitHeight: contents.implicitHeight + 24

    ColumnLayout {
        id: contents

        spacing: 10

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 12
        }

        RowLayout {
            Text {
                text: (card.index + 1).toString().padStart(2, "0")
                color: LibraryStyle.muted
                font.pixelSize: 11
            }

            Text {
                text: card.schema.label
                color: LibraryStyle.text
                font.weight: Font.Medium
                Layout.fillWidth: true
            }

            LibraryButton {
                iconName: "chevron-up"
                hint: "Move action up"
                enabled: card.index > 0

                onClicked: card.moveRequested(-1)
            }

            LibraryButton {
                iconName: "chevron-down"
                hint: "Move action down"
                enabled: card.index + 1 < card.actionCount

                onClicked: card.moveRequested(1)
            }

            LibraryButton {
                iconName: "trash-2"
                hint: "Remove action"

                onClicked: card.removeRequested()
            }
        }

        Repeater {
            model: card.schema.fields

            RowLayout {
                id: field

                required property var modelData
                readonly property string kind: modelData.kind
                readonly property var options: kind === "item" ? card.controller.serviceItems.map(
                                                                     row => ({
                                                                         text: row.title,
                                                                         value: row.id
                                                                     })) : (modelData.options
                || []).map(value => ({
                    text: value === "default" ? "Standby media" : value,
                    value: value
                }))

                Layout.fillWidth: true

                Text {
                    text: field.modelData.label
                    color: LibraryStyle.muted
                    Layout.preferredWidth: 150
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }

                LibraryCombo {
                    visible: field.kind === "choice" || field.kind === "item"
                    Layout.fillWidth: true
                    model: field.options
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: field.options.findIndex(row => row.value
                                                                 === card.payload[field.modelData.key])
                    Accessible.name: field.modelData.label

                    onActivated: card.edited(field.modelData.key, currentValue)
                }

                LibrarySpinBox {
                    visible: field.kind === "number"
                    from: field.modelData.min || 0
                    to: field.modelData.max || 1000
                    value: Number(card.payload[field.modelData.key] || from)
                    editable: true
                    Accessible.name: field.modelData.label

                    onValueModified: card.edited(field.modelData.key, value)
                }

                LibraryField {
                    visible: !["choice", "item", "number"].includes(field.kind)
                    Layout.fillWidth: true
                    text: String(card.payload[field.modelData.key] || "")
                    maximumLength: field.modelData.max || 1000
                    placeholderText: field.modelData.label
                    Accessible.name: field.modelData.label

                    onTextEdited: card.edited(field.modelData.key, text)
                }

                LibraryButton {
                    visible: field.kind === "file" || field.kind === "image"
                    iconName: "upload"
                    hint: "Choose file"

                    onClicked: card.browseRequested(field.kind)
                }
            }
        }

        Text {
            visible: card.actionType === "http" || card.actionType === "osc"
            text: card.actionType === "http"
                  ? "Runs an HTTP request when this cue is executed. Redirects are not followed." :
                    "Sends one OSC message. Arguments example: [1, \"go\", 0.5]"
            color: LibraryStyle.muted
            font.pixelSize: 11
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
