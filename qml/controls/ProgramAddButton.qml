pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The primary action targets the displayed program; menu choices add immediately.
Rectangle {
    id: control

    required property var store
    property bool ready: true
    property string selectedProgramId: ""
    property string observedProgramId: ""
    readonly property var programs: store ? store.programs : []
    readonly property string targetId: programs.some(program => program.id === selectedProgramId)
                                       ? selectedProgramId : store ? store.id : ""
    readonly property string targetTitle: (programs.find(program => program.id === targetId)
                                           || {}).title || "Choose a program"
    readonly property var alternatives: programs.filter(program => program.id !== targetId)

    signal addRequested(string programId)

    function chooseProgram(programId) {
        if (!ready || !programs.some(program => program.id === programId))
            return;
        selectedProgramId = programId;
        InteractionFeedback.play("select");
        choices.close();
        addRequested(programId);
    }

    implicitWidth: Math.min(360, caption.implicitWidth + 60)
    implicitHeight: LibraryStyle.controlHeight
    color: LibraryStyle.field
    border.color: LibraryStyle.border
    radius: LibraryStyle.controlRadius
    opacity: enabled ? 1 : 0.45
    Accessible.role: Accessible.Grouping
    Accessible.name: "Add to program: " + targetTitle

    Component.onCompleted: observedProgramId = store ? store.id : ""
    onStoreChanged: {
        selectedProgramId = "";
        observedProgramId = store ? store.id : "";
    }

    Connections {
        function onChanged() {
            if (control.store.id !== control.observedProgramId) {
                control.observedProgramId = control.store.id;
                control.selectedProgramId = "";
            }
        }

        target: control.store
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        LibraryButton {
            objectName: control.objectName + "_add"
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            quiet: true
            enabled: control.ready && control.targetId.length > 0
            hint: "Add to program: " + control.targetTitle

            contentItem: Text {
                id: caption

                renderType: Text.QtRendering
                leftPadding: 10
                rightPadding: 8
                text: "Add to program: " + control.targetTitle
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }

            onClicked: control.addRequested(control.targetId)
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            color: LibraryStyle.border
        }

        LibraryButton {
            objectName: control.objectName + "_menu"
            implicitWidth: 30
            Layout.fillHeight: true
            quiet: true
            iconName: "chevron-down"
            enabled: control.ready && control.programs.length > 0
            hint: "Choose another program"

            onClicked: choices.open()
        }
    }

    Popup {
        id: choices

        function placeMenu() {
            const overlay = control.Overlay.overlay;
            if (!overlay)
                return;
            const point = control.mapToItem(overlay, 0, 0);
            choices.y = point.y + control.height + choices.implicitHeight + 8 > overlay.height ?
                        -choices.implicitHeight - 6 : control.height + 6;
        }

        objectName: control.objectName + "_choices"
        focus: true
        y: control.height + 6
        width: Math.max(260, Math.min(380, control.width))
        x: control.width - width
        padding: 6
        margins: 8
        implicitHeight: control.alternatives.length ? Math.min(300, control.alternatives.length
                                                               * 48 + (control.alternatives.length
                                                                       - 1) * 4 + 12) : 48

        background: Rectangle {
            radius: 8
            color: LibraryStyle.panel
            border.color: LibraryStyle.border
        }
        contentItem: ListView {
            id: menuItems

            focus: true
            clip: true
            spacing: 4
            model: control.alternatives
            implicitHeight: contentHeight

            ScrollBar.vertical: ScrollBar {
            }
            delegate: LibraryButton {
                id: destination

                required property var modelData

                width: menuItems.width
                height: 48
                leftPadding: 12
                rightPadding: 12
                topPadding: 8
                bottomPadding: 8
                quiet: true
                selected: ListView.isCurrentItem && menuItems.activeFocus
                hint: "Add to " + modelData.title + (modelData.date ? " · " + modelData.date : "")
                // The row already shows the full action; a tooltip obscures nearby menu entries.
                ToolTip.visible: false

                contentItem: Item {
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                        spacing: 4

                        Text {
                            renderType: Text.QtRendering
                            width: parent.width
                            text: destination.modelData.title
                            textFormat: Text.PlainText
                            color: LibraryStyle.text
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        Text {
                            renderType: Text.QtRendering
                            width: parent.width
                            text: destination.modelData.date || "No date"
                            color: LibraryStyle.muted
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }
                }

                onClicked: control.chooseProgram(modelData.id)
            }

            Keys.onReturnPressed: {
                if (currentIndex >= 0 && currentIndex < control.alternatives.length)
                    control.chooseProgram(control.alternatives[currentIndex].id);
            }
            Keys.onEnterPressed: {
                if (currentIndex >= 0 && currentIndex < control.alternatives.length)
                    control.chooseProgram(control.alternatives[currentIndex].id);
            }

            Text {
                renderType: Text.QtRendering
                anchors.centerIn: parent
                visible: control.alternatives.length === 0
                text: "No other programs"
                color: LibraryStyle.muted
                font.pixelSize: 12
            }
        }

        onAboutToShow: placeMenu()
        onOpened: {
            menuItems.currentIndex = control.alternatives.length ? 0 : -1;
            placeMenu();
        }
    }
}
