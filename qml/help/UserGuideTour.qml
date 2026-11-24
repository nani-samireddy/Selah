pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import "GuideTopics.js" as GuideTopics

// Nonmodal coaching: users can try the highlighted controls without losing the guide.
Item {
    id: tour

    required property var host
    property bool running: false
    property int topicIndex: 0
    property int stepIndex: 0
    property rect targetRect: Qt.rect(0, 0, 0, 0)
    readonly property var topic: GuideTopics.topics[topicIndex]
    readonly property var step: topic.steps[stepIndex]
    readonly property var completed: JSON.parse(progress.completedIds)

    signal guideRequested(int index)

    function start(index) {
        topicIndex = index;
        stepIndex = 0;
        running = true;
        reveal();
    }

    function reveal() {
        targetRect = Qt.rect(0, 0, 0, 0);
        if (step.library)
            host.browseLibrary(step.library);
        else if (step.outputs)
            host.showOutputs(step.outputs);
        else
            host.switchWorkspace(step.workspace);
        Qt.callLater(refreshTarget);
    }

    function move(delta) {
        stepIndex = Math.max(0, Math.min(topic.steps.length - 1, stepIndex + delta));
        reveal();
    }

    function finish(complete = false) {
        if (complete && !completed.includes(topic.id))
            progress.completedIds = JSON.stringify(completed.concat([topic.id]));
        running = false;
        targetRect = Qt.rect(0, 0, 0, 0);
        // Keep the current workspace and any work the user chose to do there.
    }

    function findControl(item, name) {
        if (!item || !item.visible)
            return null;
        if (item.objectName === name)
            return item;
        for (const child of item.children || []) {
            const found = findControl(child, name);
            if (found)
                return found;
        }
        return null;
    }

    function refreshTarget() {
        if (!running)
            return;
        const target = findControl(host.header, step.target) || findControl(host.contentItem, step.target)
              || findControl(Overlay.overlay, step.target);

        if (!target) {
            targetRect = Qt.rect(0, 0, 0, 0);
            return;
        }
        const position = target.mapToItem(tour, 0, 0);
        targetRect = Qt.rect(position.x - 5, position.y - 5, target.width + 10, target.height + 10);
    }

    objectName: "interactiveUserGuide"
    parent: Overlay.overlay
    anchors.fill: parent
    z: 1000
    visible: running

    Settings {
        id: progress

        property string completedIds: "[]"

        category: "UserGuide"
    }

    Timer {
        interval: 120
        repeat: true
        running: tour.running

        onTriggered: tour.refreshTarget()
    }

    Rectangle {
        objectName: "guideControlHighlight"
        x: tour.targetRect.x
        y: tour.targetRect.y
        width: tour.targetRect.width
        height: tour.targetRect.height
        visible: width > 0 && height > 0
        radius: 7
        color: "transparent"
        border.width: 2
        border.color: LibraryStyle.dark ? "#C9B0F3" : "#735099"
    }

    StudioPanel {
        id: card

        objectName: "guideStepCard"
        surfaceOpacity: 1
        width: Math.min(390, tour.width - 32)
        height: contents.implicitHeight + 32
        anchors.right: parent.right
        anchors.rightMargin: 20
        // Shift away from a highlighted bottom-right control.
        y: tour.targetRect.width > 0 && tour.targetRect.x + tour.targetRect.width > tour.width
           - width - 30 && tour.targetRect.y + tour.targetRect.height > tour.height - height - 30
           ? 20 : tour.height - height - 20

        ColumnLayout {
            id: contents

            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                LibraryCaption {
                    text: "WALKTHROUGH · " + (tour.stepIndex + 1) + " / " + tour.topic.steps.length
                    Layout.fillWidth: true
                }

                StudioButton {
                    objectName: "guideEndButton"
                    iconName: "x"
                    implicitWidth: 28
                    quiet: true
                    hint: "End walkthrough"

                    onClicked: tour.finish()
                }
            }

            Text {
                text: tour.step.title
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 18
                font.weight: Font.Medium
                color: LibraryStyle.text
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Text {
                text: tour.step.body
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 13
                color: LibraryStyle.text
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            ProgressBar {
                from: 0
                to: tour.topic.steps.length
                value: tour.stepIndex + 1
                Layout.fillWidth: true
                implicitHeight: 4
            }

            RowLayout {
                Layout.fillWidth: true

                LibraryButton {
                    text: "Read guide"
                    quiet: true

                    onClicked: {
                        const index = tour.topicIndex;
                        tour.finish();
                        tour.guideRequested(index);
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                LibraryButton {
                    objectName: "guidePreviousStep"
                    iconName: "chevron-left"
                    hint: "Previous step"
                    implicitWidth: 30
                    enabled: tour.stepIndex > 0

                    onClicked: tour.move(-1)
                }

                LibraryButton {
                    objectName: "guideNextStep"
                    text: tour.stepIndex === tour.topic.steps.length - 1 ? "Finish" : "Next"
                    accented: true

                    onClicked: {
                        if (tour.stepIndex === tour.topic.steps.length - 1)
                            tour.finish(true);
                        else
                            tour.move(1);
                    }
                }
            }
        }
    }
}
