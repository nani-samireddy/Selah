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
    property string readingHtml: ""
    property string search: ""
    readonly property var topics: GuideTopics.topics
    readonly property var topic: topics[topicIndex]
    readonly property bool completed: tour.completed.includes(topic.id)

    function matchesTopic(topic) {
        return !search.length || (topic.title + " " + topic.summary + " " + topic.steps.map(step
                                                                                            => step.body).join(
                                      " ")).toLowerCase().includes(search);
    }

    function iconFor(id) {
        return ({
                    "first-service": "play",
                    songs: "type",
                    bible: "book-open",
                    design: "pen-tool",
                    present: "monitor-play",
                    remote: "smartphone"
                })[id] || "book-open";
    }

    function loadTopic() {
        body = GuideContent.read(topic.file) || "This guide could not be loaded.";
        readingHtml = GuideContent.formatted(topic.file) || body;
        readingArea.cursorPosition = 0;
        Qt.callLater(() => {
            const viewport = readingScroll.contentItem as Flickable;
            if (viewport)
                viewport.contentY = 0;
        });
    }

    objectName: "userGuideDialog"
    title: "Learn Selah"
    modal: true
    parent: Overlay.overlay
    width: Math.min(1000, parent.width - 48)
    height: Math.min(740, parent.height - 48)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 24
    standardButtons: Dialog.NoButton

    background: SquircleSurface {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 16
    }
    header: Item {
        implicitHeight: 72

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 20
            spacing: 12

            StudioIcon {
                name: "book-open"
                tone: LibraryStyle.text
                Layout.minimumWidth: 18
                Layout.maximumWidth: 18
                Layout.preferredHeight: 18
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3

                Text {
                    text: "Learn Selah"
                    Layout.fillWidth: true
                    color: LibraryStyle.text
                    font.family: LibraryStyle.fontFamily
                    font.pixelSize: 17
                    font.weight: Font.Medium
                }

                LibraryCaption {
                    text: "Guides & walkthroughs"
                    Layout.fillWidth: true
                    font.pixelSize: 11
                }
            }

            StudioButton {
                objectName: "userGuideClose"
                Layout.minimumWidth: 28
                Layout.maximumWidth: 28
                iconName: "x"
                quiet: true
                hint: "Close guides"

                onClicked: guide.close()
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: LibraryStyle.divider
        }
    }
    contentItem: RowLayout {
        spacing: 24

        ColumnLayout {
            Layout.preferredWidth: 210
            Layout.maximumWidth: 210
            Layout.fillHeight: true
            spacing: 12

            LibraryField {
                objectName: "userGuideSearch"
                Layout.fillWidth: true
                leadingIcon: "search"
                placeholderText: "Find a guide…"

                onTextChanged: {
                    guide.search = text.trim().toLowerCase();
                    if (!guide.matchesTopic(guide.topic)) {
                        const next = guide.topics.findIndex(topic => guide.matchesTopic(topic));
                        if (next >= 0)
                            guide.topicIndex = next;
                    }
                }
            }

            Text {
                text: "EXPLORE"
                color: LibraryStyle.muted
                font.pixelSize: 10
                font.letterSpacing: 1
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true

                Column {
                    width: parent.width
                    spacing: 5

                    Repeater {
                        model: guide.topics

                        ItemDelegate {
                            id: entry

                            required property var modelData
                            required property int index

                            objectName: "guideTopic_" + modelData.id
                            width: parent.width
                            height: visible ? 44 : 0
                            visible: guide.matchesTopic(modelData)
                            padding: 10
                            Accessible.name: modelData.title

                            background: SquircleSurface {
                                radius: 7
                                color: guide.topicIndex === entry.index ? LibraryStyle.hover :
                                                                          entry.hovered
                                                                          ? LibraryStyle.field :
                                                                            "transparent"
                                border.color: guide.topicIndex === entry.index
                                              ? LibraryStyle.border : "transparent"
                            }
                            contentItem: RowLayout {
                                spacing: 9

                                StudioIcon {
                                    name: guide.iconFor(entry.modelData.id)
                                    tone: guide.topicIndex === entry.index ? LibraryStyle.text :
                                                                             LibraryStyle.muted
                                }

                                Text {
                                    text: entry.modelData.title
                                    font.family: LibraryStyle.fontFamily
                                    font.pixelSize: 12
                                    color: guide.topicIndex === entry.index ? LibraryStyle.text :
                                                                              LibraryStyle.muted
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }

                            onClicked: guide.topicIndex = index
                        }
                    }

                    LibraryCaption {
                        visible: !guide.topics.some(topic => guide.matchesTopic(topic))
                        text: "No guides found. Try another search."
                        width: parent.width
                        wrapMode: Text.Wrap
                    }
                }
            }

            RowLayout {
                spacing: 7

                StudioIcon {
                    name: "book-open"
                    tone: LibraryStyle.muted
                }

                LibraryCaption {
                    text: "Always available offline"
                    font.pixelSize: 11
                }
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
                font.pixelSize: 23
                font.weight: Font.Medium
                color: LibraryStyle.text
                Layout.fillWidth: true
            }

            LibraryCaption {
                text: guide.topic.summary
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            SquircleSurface {
                Layout.fillWidth: true
                implicitHeight: 76
                color: LibraryStyle.field
                border.color: LibraryStyle.border
                radius: 10

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Text {
                            text: "Interactive walkthrough"
                            Layout.fillWidth: true
                            color: LibraryStyle.text
                            font.family: LibraryStyle.fontFamily
                            font.pixelSize: 13
                            font.weight: Font.Medium
                        }

                        LibraryCaption {
                            Layout.fillWidth: true
                            text: guide.topic.steps.length + " steps · " + (guide.completed
                                                                            ? "Completed" :
                                                                              "Follow along in the app")
                            font.pixelSize: 11
                        }
                    }

                    LibraryButton {
                        objectName: "startGuideWalkthrough"
                        Layout.maximumWidth: implicitWidth
                        iconName: "play"
                        text: guide.completed ? "Replay" : "Start walkthrough"
                        accented: true

                        onClicked: {
                            const index = guide.topicIndex;
                            guide.close();
                            Qt.callLater(() => guide.tour.start(index));
                        }
                    }
                }
            }

            ScrollView {
                id: readingScroll

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Item {
                    width: readingScroll.availableWidth
                    height: readingArea.implicitHeight

                    TextArea {
                        id: readingArea

                        objectName: "userGuideReadingArea"
                        width: Math.min(640, parent.width - 8)
                        anchors.horizontalCenter: parent.horizontalCenter
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.RichText
                        text: guide.readingHtml
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
