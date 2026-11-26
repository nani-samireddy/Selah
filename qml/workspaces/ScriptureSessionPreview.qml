pragma ComponentBehavior: Bound
import QtQuick

Rectangle {
    id: preview

    required property var controller
    property int page: 0
    readonly property var slide: controller ? controller.slides[page] || ({}) : ({})

    color: "#090909"
    clip: true

    SlideRenderer {
        anchors.centerIn: parent
        scale: Math.min(preview.width / 1920, preview.height / 1080)
        elements: preview.slide.elements || []
        backgroundColor: preview.slide.background || "#000000"
        backgroundFill: preview.slide.backgroundFill || ({})
        ambientOpacity: preview.slide.ambientOpacity || 0
        videoEnabled: false
        playing: false
        audible: false
    }

    LibraryCaption {
        anchors.centerIn: parent
        visible: !preview.controller || !preview.controller.slides.length
        text: "Select a verse to preview"
    }

    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: !!preview.controller && preview.controller.slides.length > 1

        LibraryButton {
            iconName: "chevron-left"
            enabled: preview.page > 0

            onClicked: --preview.page
        }

        LibraryButton {
            text: (preview.page + 1) + " / " + (preview.controller
                                                ? preview.controller.slides.length : 0)
            iconName: "chevron-right"
            enabled: !!preview.controller && preview.page + 1 < preview.controller.slides.length

            onClicked: ++preview.page
        }
    }

    Connections {
        function onChanged() {
            preview.page = 0;
        }

        target: preview.controller
    }
}
