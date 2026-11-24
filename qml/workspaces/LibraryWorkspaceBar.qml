pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: bar

    required property var host

    implicitHeight: 64
    color: LibraryStyle.background

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: LibraryStyle.tone("#2C2C2C")
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 8

        LibraryTabs {
            options: [
                {
                    text: "Songs",
                    value: "songs"
                },
                {
                    text: "Bible",
                    value: "bible"
                },
                {
                    text: "Media",
                    value: "media"
                },
                {
                    text: "Presentations",
                    value: "custom"
                },
                {
                    text: "Templates",
                    value: "templates"
                }
            ]
            currentValue: bar.host.librarySection

            onChosen: value => bar.host.showLibrarySection(value)
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            text: "Add content to"
            color: LibraryStyle.tone("#909090")
            font.family: LibraryStyle.fontFamily
            font.pixelSize: 12
            font.letterSpacing: 0
            visible: !["templates", "custom"].includes(bar.host.librarySection)
        }

        LibraryCombo {
            visible: !["templates", "custom"].includes(bar.host.librarySection)
            Layout.preferredWidth: Math.min(300, bar.width * 0.29)
            model: bar.host.programs.id ? [
                                              {
                                                  label: "Program",
                                                  value: "program"
                                              },
                                              {
                                                  label: "Design presentation",
                                                  value: "design"
                                              }
                                          ] : [
                                              {
                                                  label: "Design presentation",
                                                  value: "design"
                                              }
                                          ]
            textRole: "label"
            currentIndex: Math.max(0, model.findIndex(item => item.value
                                                              === bar.host.libraryDestination))

            onActivated: index => {
                bar.host.libraryDestination = model[index].value;
                bar.host.refreshLibraryTarget();
            }
        }

        Text {
            visible: ["templates", "custom"].includes(bar.host.librarySection)
            text: bar.host.librarySection === "custom" ? "Saved presentations · Edit in Design" :
                                                         "Reusable layouts for songs and scripture"
            color: LibraryStyle.tone("#929292")
            font.pixelSize: 12
        }
    }
}
