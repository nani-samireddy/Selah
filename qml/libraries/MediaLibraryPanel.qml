pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: panel

    property var library
    property var programTarget: null
    property var document
    property string kindFilter: "all"
    property var filteredAssets: {
        let query = search.text.toLowerCase();
        return library.assets.filter(a => (kindFilter === "all" || a.kind === kindFilter)
                && a.name.toLowerCase().includes(query));
    }

        signal importRequested

        width: Math.min(880, parent.width - 60)
        height: Math.min(630, parent.height - 40)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        focus: true
        padding: 20

        background: Rectangle {
        color: "#191919"
        border.color: "#3B3B3B"
        radius: 12
    }

        onOpened: {
        library.refresh();
        search.forceActiveFocus();
    }

        ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
        Layout.fillWidth: true

        Column {
        spacing: 5

        Text {
        text: "Media library"
        color: "#ECECEC"
        font.pixelSize: 20
        font.weight: Font.Medium
    }

        Text {
        text: "Import once. Reuse across presentations."
        color: "#929292"
        font.pixelSize: 12
    }
    }

        Item {
        Layout.fillWidth: true
    }

        StudioButton {
        text: panel.document.importing ? "Importing…" : "Import media"
        iconName: "plus"
        enabled: !panel.document.importing

        onClicked: panel.importRequested()
    }

        StudioButton {
        text: "Close"
        quiet: true

        onClicked: panel.close()
    }
    }

        RowLayout {
        Layout.fillWidth: true

        TextField {
        id: search

        Layout.fillWidth: true
        placeholderText: "Search media…"
        color: "#DDDDDD"
        placeholderTextColor: "#777777"
        font.pixelSize: 12
        selectByMouse: true
        padding: 10

        background: Rectangle {
        color: "#232323"
        border.color: search.activeFocus ? "#888888" : "#373737"
        radius: 6
    }
    }

        Repeater {
        model: [
        {
        title: "All",
        kind: "all"
    },
        {
        title: "Images",
        kind: "image"
    },
        {
        title: "Videos",
        kind: "video"
    }
        ]

        StudioButton {
        required property var modelData

        text: modelData.title
        checked: panel.kindFilter === modelData.kind

        onClicked: panel.kindFilter = modelData.kind
    }
    }
    }

        Text {
        visible: panel.library.error.length > 0
        text: panel.library.error
        color: "#EAA5A5"
        wrapMode: Text.Wrap
        Layout.fillWidth: true
        font.pixelSize: 12
    }

        ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        GridLayout {
        width: parent.width
        columns: 3
        columnSpacing: 14
        rowSpacing: 18

        Repeater {
        model: panel.filteredAssets

        delegate: ColumnLayout {
        id: asset

        required property var modelData

        Layout.fillWidth: true
        Layout.preferredWidth: 220
        spacing: 7

        Rectangle {
        Layout.fillWidth: true
        implicitHeight: 118
        radius: 7
        color: "#242424"
        border.color: "#383838"
        clip: true

        Image {
        anchors.fill: parent
        anchors.margins: 1
        source: asset.modelData.thumbnail
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }

        StudioIcon {
        visible: !asset.modelData.thumbnail
        anchors.centerIn: parent
        name: asset.modelData.kind === "video" ? "play" : "image"
        width: 24
        height: 24
    }

        Rectangle {
        width: tag.implicitWidth + 12
        height: 21
        color: "#D0181818"
        radius: 4

        anchors {
        left: parent.left
        bottom: parent.bottom
        margins: 7
    }

        Text {
        id: tag

        anchors.centerIn: parent
        text: !asset.modelData.available ? "Missing" : asset.modelData.kind === "video" ? "Video" :
        "Image"
        font.pixelSize: 10
        color: "#DDDDDD"
    }
    }
    }

        Text {
        text: asset.modelData.name
        color: "#D8D8D8"
        font.pixelSize: 12
        elide: Text.ElideMiddle
        Layout.fillWidth: true
    }

        RowLayout {
        Layout.fillWidth: true

        Text {
        text: (asset.modelData.bytes / 1048576).toFixed(1) + " MB"
        color: "#777777"
        font.pixelSize: 10
        Layout.fillWidth: true
    }

        StudioButton {
        text: panel.programTarget ? "Add to program" : "Add to slide"
        implicitHeight: 28
        enabled: asset.modelData.available

        onClicked: {
        if (panel.programTarget ? panel.programTarget.addMedia(asset.modelData) :
        panel.document.useMedia(asset.modelData))
        panel.close();
    }
    }
    }
    }
    }
    }
    }

        Text {
        visible: panel.filteredAssets.length === 0
        text: search.text.length ? "No matching media." :
        "Your library is empty. Import an image or video to get started."
        color: "#999999"
        font.pixelSize: 12
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
    }

        Text {
        text: panel.library.assets.length + " assets · Stored on this computer"
        color: "#777777"
        font.pixelSize: 10
    }
    }
    }
