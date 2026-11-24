pragma ComponentBehavior: Bound
import QtQuick

// A frozen GPU texture bends into the toolbar; editor controls remain real QML items.
Item {
    id: genie

    required property Item panel
    property point targetPoint
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
        motion.duration = Math.max(90, Math.round(440 * Math.abs(destination - progress)));
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

        anchors.fill: parent
        sourceItem: genie.supported ? genie.panel : null
        live: false
        hideSource: genie.animating
        visible: false
    }

    ShaderEffect {
        property var source: frozenPanel
        property real progress: genie.progress
        property vector2d panelSize: Qt.vector2d(width, height)
        property vector2d targetPoint: Qt.vector2d(genie.targetPoint.x, genie.targetPoint.y)

        objectName: "slidesPanelGenieEffect"
        anchors.fill: parent
        visible: genie.animating && genie.supported
        vertexShader: "qrc:/shaders/panel-genie.vert.qsb"
        fragmentShader: "qrc:/shaders/panel-genie.frag.qsb"

        mesh: GridMesh {
            resolution: Qt.size(32, 24)
        }
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
