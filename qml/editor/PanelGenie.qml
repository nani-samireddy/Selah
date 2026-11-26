pragma ComponentBehavior: Bound
import QtQuick

// Compress the expanded panel into its thumbnail strip, crossfading at the end.
Item {
    id: genie

    required property Item panel
    property rect targetRect
    property bool expanded: true
    property bool motionEnabled: true
    property real progress: 0
    property bool ready: false
    readonly property bool supported: GraphicsInfo.api !== GraphicsInfo.Software
    readonly property bool animating: motion.running

    function synchronize() {
        const destination = expanded ? 0 : 1;
        const interrupted = motion.running;
        motion.stop();
        if (!ready || !motionEnabled || !supported || InteractionFeedback.reducedMotion) {
            progress = destination;
            return;
        }
        if (!interrupted)
            frozenPanel.scheduleUpdate();
        motion.from = progress;
        motion.to = destination;
        motion.duration = Math.max(90, Math.round(320 * Math.abs(destination - progress)));
        motion.start();
    }

    x: panel.x
    y: panel.y
    width: panel.width
    height: panel.height

    onExpandedChanged: synchronize()
    onMotionEnabledChanged: synchronize()
    Component.onCompleted: {
        progress = expanded ? 0 : 1;
        ready = true;
    }

    ShaderEffectSource {
        id: frozenPanel

        x: genie.targetRect.x * genie.progress
        y: genie.targetRect.y * genie.progress
        width: genie.width + (genie.targetRect.width - genie.width) * genie.progress
        height: genie.height + (genie.targetRect.height - genie.height) * genie.progress
        sourceItem: genie.supported ? genie.panel : null
        live: false
        hideSource: genie.animating
        visible: genie.animating && genie.supported
        opacity: 1 - genie.progress
        smooth: true
    }

    MouseArea {
        anchors.fill: parent
        enabled: genie.animating
        acceptedButtons: Qt.AllButtons

        onWheel: event => event.accepted = true
    }

    NumberAnimation {
        id: motion

        objectName: "slidesPanelAnimationClock"
        target: genie
        property: "progress"
        easing.type: Easing.InOutCubic
    }
}
