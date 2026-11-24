pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "GuideTopics.js" as GuideTopics

Dialog {
    id: guide

    required property var tour
    property int topicIndex: 0
    property string body: ""
    property string search: ""
    readonly property var topics: GuideTopics.topics
    readonly property var topic: topics[topicIndex]

    function matchesTopic(topic) {
        return !search.length || (topic.title + " " + topic.summary + " " + topic.steps.map(step
                                                                                            => step.body).join(
                                      " ")).toLowerCase().includes(search);
    }

    function loadTopic() {
        body = GuideContent.read(topic.file)
                || "This guide could not be loaded. You can still start the walkthrough.";
        readingArea.cursorPosition = 0;
    }

    objectName: "userGuideDialog"
    title: "Learn Selah"
    modal: true
    parent: Overlay.overlay
    width: Math.min(980, parent.width - 48)
    height: Math.min(740, parent.height - 48)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 20
    standardButtons: Dialog.Close

    background: Rectangle {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 12
    }
    contentItem: ColumnLayout {
        spacing: 18

        LibraryField {
            objectName: "userGuideSearch"
            Layout.fillWidth: true
            leadingIcon: "search"
            placeholderText: "Find a guide: songs, Bible, displays, fonts…"

            onTextChanged: guide.search = text.trim().toLowerCase()
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 22

            ColumnLayout {
                Layout.preferredWidth: 220
                Layout.alignment: Qt.AlignTop
                spacing: 6

                Repeater {
                    model: guide.topics

                    LibraryButton {
                        required property var modelData
                        required property int index

                        objectName: "guideTopic_" + modelData.id
                        visible: guide.matchesTopic(modelData)
                        Layout.fillWidth: true
                        text: modelData.title
                        selected: guide.topicIndex === index
                        quiet: !selected

                        onClicked: guide.topicIndex = index
                    }
                }

                LibraryCaption {
                    visible: !guide.topics.some(topic => guide.matchesTopic(topic))
                    text: "No matching guides. Try another search."
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                LibraryCaption {
                    Layout.fillWidth: true
                    text: "Read offline, or follow a walkthrough in the app. You stay in control of changes and live output."
                    wrapMode: Text.Wrap
                }
            }

            Rectangle {
                Layout.fillHeight: true
                implicitWidth: 1
                color: LibraryStyle.divider
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                Text {
                    text: guide.topic.title
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 22
                    font.weight: Font.Medium
                    color: LibraryStyle.text
                    Layout.fillWidth: true
                }

                LibraryCaption {
                    text: guide.topic.summary
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    LibraryButton {
                        objectName: "startGuideWalkthrough"
                        iconName: "play"
                        text: guide.tour.completed.includes(guide.topic.id) ? "Replay walkthrough" :
                                                                              "Start walkthrough"
                        accented: true

                        onClicked: {
                            const index = guide.topicIndex;
                            guide.close();
                            Qt.callLater(() => guide.tour.start(index));
                        }
                    }

                    LibraryCaption {
                        text: guide.topic.steps.length + " steps" + (guide.tour.completed.includes(
                                                                         guide.topic.id)
                                                                     ? " · Completed" : "")
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    TextArea {
                        id: readingArea

                        objectName: "userGuideReadingArea"
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.MarkdownText
                        text: guide.body
                        wrapMode: TextEdit.Wrap
                        font.family: LibraryStyle.fontFamily
                        font.pixelSize: 13
                        color: LibraryStyle.text
                        background: null
                        padding: 0
                    }
                }
            }
        }
    }

    onOpened: loadTopic()
    onTopicIndexChanged: if (opened)
                             loadTopic()
}
