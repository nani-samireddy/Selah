pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: panel

    required property var controller
    property string feedback: ""

    color: LibraryStyle.panel
    radius: 10
    border.color: LibraryStyle.divider
    implicitHeight: body.implicitHeight + 24

    FileDialog {
        id: chooseImage

        title: "Choose default image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif *.svg)"]

        onAccepted: panel.feedback = panel.controller.setDefaultImage(selectedFile) ? "" :
                                                                                      panel.controller.status
    }

    ColumnLayout {
        id: body

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 8

        RowLayout {
            StudioIcon {
                name: "image"
            }

            Text {
                text: "Default image"
                color: LibraryStyle.text
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            StudioButton {
                iconName: "trash-2"
                hint: "Remove default image preference"
                quiet: true
                visible: panel.controller.defaultImageSource.toString().length > 0

                onClicked: panel.controller.removeDefaultImage()
            }
        }

        Rectangle {
            id: imageFrame

            Layout.fillWidth: true
            Layout.preferredHeight: width * 9 / 16
            color: "#000000"
            radius: 5
            clip: true

            Image {
                anchors.fill: parent
                source: panel.controller.defaultImageSource
                fillMode: panel.controller.defaultImageFit === "cover" ? Image.PreserveAspectCrop :
                                                                         Image.PreserveAspectFit
                asynchronous: true
                autoTransform: true
                sourceSize.width: 640
                sourceSize.height: 360
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 24
                visible: !panel.controller.defaultImageAvailable
                text: panel.controller.defaultImageSource.toString().length
                      ? "Saved image is missing" : "Church logo or welcome image"
                color: "#969696"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }

        Text {
            text: panel.controller.defaultImageName || "Show between service items"
            color: LibraryStyle.muted
            font.pixelSize: 10
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }

        RowLayout {
            StudioButton {
                text: panel.controller.defaultImageAvailable ? "Change" : "Choose image"
                iconName: "upload"
                Layout.fillWidth: true

                onClicked: chooseImage.open()
            }

            StudioButton {
                text: "Fit"
                checked: panel.controller.defaultImageFit === "contain"
                hint: "Fit the whole image"
                enabled: panel.controller.defaultImageAvailable

                onClicked: panel.controller.defaultImageFit = "contain"
            }

            StudioButton {
                text: "Fill"
                checked: panel.controller.defaultImageFit === "cover"
                hint: "Fill the screen, cropping the edges"
                enabled: panel.controller.defaultImageAvailable

                onClicked: panel.controller.defaultImageFit = "cover"
            }
        }

        StudioButton {
            objectName: "showDefaultImageButton"
            text: panel.controller.defaultImageShowing ? (panel.controller.liveCount
                                                          ? "Resume presentation" :
                                                            "Hide default image") :
                                                         "Show default image"
            iconName: panel.controller.defaultImageShowing ? "play" : "image"
            checked: panel.controller.defaultImageShowing
            accented: !panel.controller.defaultImageShowing
            enabled: panel.controller.defaultImageAvailable || panel.controller.defaultImageShowing
            Layout.fillWidth: true

            onClicked: panel.feedback = panel.controller.showDefaultImage(
                           !panel.controller.defaultImageShowing) ? "" : panel.controller.status
        }

        Text {
            visible: panel.feedback.length > 0 || panel.controller.defaultImageShowing
            text: panel.feedback || "Image changes apply the next time you show it."
            color: panel.feedback ? (LibraryStyle.dark ? "#D4B993" : "#7D572A") : LibraryStyle.muted
            font.pixelSize: 10
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
    }
}
