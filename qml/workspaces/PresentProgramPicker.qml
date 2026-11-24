pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: picker

    required property var host
    property string message: ""
    property string choiceId: ""
    readonly property var programs: host.programs.programs.map(record => ({
        id: record.id,
        label: record.title + (record.date ? " · " + record.date : "")
    }))

    Layout.fillWidth: true
    spacing: 6

    onVisibleChanged: if (visible)
                          choiceId = host.programs.id

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        LibraryCaption {
            text: "Program"
        }

        LibraryCombo {
            id: selection

            objectName: "presentProgramSelector"
            Layout.fillWidth: true
            Layout.maximumWidth: 420
            model: picker.programs
            textRole: "label"
            currentIndex: {
                const current = picker.programs.findIndex(record => record.id === (picker.choiceId
                                                                                   || picker.host.programs.id));
                return current >= 0 ? current : (picker.programs.length ? 0 : -1);
            }
            enabled: model.length > 0
            displayText: count ? currentText : "No saved programs"
            Accessible.name: "Program to present"

            onActivated: index => {
                picker.choiceId = picker.programs[index].id;
                picker.message = "";
            }
        }

        LibraryButton {
            objectName: "loadPresentProgramButton"
            implicitWidth: implicitHeight
            hint: (picker.host.outputs.audienceEnabled || (picker.host.ndi.active &&
                                                           !picker.host.ndi.diagnostic))
                  ? "Take program live" : "Load program"
            iconName: "play"
            accented: true
            enabled: selection.currentIndex >= 0 && picker.programs.length > 0

            onClicked: {
                if (picker.host.loadProgramForPresentation(
                            picker.programs[selection.currentIndex].id))
                    picker.message = "";
                else
                    picker.message = picker.host.programs.items.length
                            ? picker.host.programs.status :
                              "This program has no slides. Add items in Programs first.";
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    LibraryCaption {
        visible: picker.message.length > 0
        text: picker.message
        Layout.fillWidth: true
        wrapMode: Text.Wrap
    }
}
