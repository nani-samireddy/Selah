pragma ComponentBehavior: Bound
import QtQuick

Window {
    id: output

    property var controller
    property string role: "audience"

    // Outputs are independent top-level windows, not dialogs owned by the editor.
    transientParent: null
    width: 960
    height: 540
    title: role === "stage" ? "Selah — Stage output" : "Selah — Audience output"
    color: "#000000"

    onClosing: controller.stop(role)

    Item {
        width: 1920
        height: 1080
        scale: Math.min(output.width / 1920, output.height / 1080)
        transformOrigin: Item.TopLeft
        x: (output.width - width * scale) / 2
        y: (output.height - height * scale) / 2

        AudienceRenderer {
            visible: output.role === "audience"
            active: output.visible && output.role === "audience"
            controller: output.controller
            audible: output.controller.audioEnabled
            videoEnabled: output.role === "audience"
        }

        Loader {
            anchors.fill: parent
            active: output.role === "stage"

            sourceComponent: Component {
                StageView {
                    controller: output.controller
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "#000000"
            visible: output.role === "audience" ? output.controller.audienceBlack :
                                                  output.controller.stageBlack
        }
    }

    Shortcut {
        sequence: "Escape"

        onActivated: output.controller.stop(output.role)
    }
}
