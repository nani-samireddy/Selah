pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: guide

    property int topicIndex: 0
    readonly property var topics: [
        {
            title: "Your first service",
            body: "# Welcome to Selah\n\nPrepare your content in **Libraries**, arrange a service in **Programs**, customize layouts in **Design**, and operate it in **Present**.\n\n1. Open Programs and create or select a program.\n2. Open Libraries and choose Songs, Bible, Media, or Custom slides.\n3. Use **Add to program** to add your content. Its arrow lets you choose a different destination.\n4. Return to Programs and drag items into service order.\n5. Open Outputs and select your audience and speaker displays.\n6. Open Present, load your program, and select a slide to show it.\n\nCheck the audience display before the service. Editing a library item does not automatically change an already prepared program snapshot."
        },
        {
            title: "Songs & sections",
            body: "# Prepare a song\n\nIn **Libraries → Songs**, choose New song, or **Quick Song** to paste a title, lyrics, and tags. Blank lines in Quick Song create separate sections.\n\nEdit sections as cards, then arrange them with Song order. Repeat a section when it should appear again. Tags help you find songs by theme or occasion.\n\nFor two languages, choose a second language and enter matching translation rows in each section. Use a bilingual lyrics template so both languages remain visible.\n\nSave the song, choose a layout and lines per slide, then Add to program. Select the destination with the arrow beside that button."
        },
        {
            title: "Bible & dual Bibles",
            body: "# Display scripture\n\nIn **Libraries → Bible**, select a translation and book. The book opens at chapter 1. Use the chapter grid to change chapters, and the verse grid to scroll to and highlight a verse. Click verse cards to select several, or Select chapter.\n\n## Two Bibles on the same slide\n\nChoose **Second Bible (optional)**. Selah matches each reference by book, chapter, and verse, including when the book names use different languages. Both texts appear in the reading list.\n\nChoose **Automatic layout** or **Dual Bible**, then Add to program. If the second Bible lacks a selected verse, choose another Bible or remove that verse. Selah will not silently substitute another reference.\n\nLong verses continue across slides. A shorter translation stays visible while the longer translation continues. Translation wording and verse numbering can differ; review the output before presenting.\n\n## Add a translation\n\nChoose Import translation to browse the GitHub catalog or import a local XML/JSON file. Download progress appears during import. Once imported, the Bible is available offline."
        },
        {
            title: "Templates & custom slides",
            body: "# Design your content\n\n**Templates** define reusable layouts for songs and scripture. **Custom slides** are presentations with their own editable content. Open either from Libraries to edit it in Design.\n\nFor dual Bibles, start from the **Dual Bible** template. In the text inspector, bind the first text box to Scripture text and the second to **Secondary scripture text**. Use Secondary scripture reference, Secondary version, and Secondary credits for the second Bible’s metadata.\n\nEach text box has independent font, size, color, alignment, and position. Keep bound boxes inside the canvas. If text cannot fit, enlarge the box or reduce the font size.\n\nSave a template and select it when adding library content to a program. Existing program snapshots keep their previous layout until you explicitly apply a new layout."
        },
        {
            title: "Displays & presenting",
            body: "# Run the service\n\nOpen **Outputs** to assign connected audience and speaker displays. Confirm the selected monitor before enabling output. The speaker view is for the operator or presenter and can include notes and upcoming content.\n\nIn Present, load a program, choose a service item, and select a slide. The live output uses the loaded snapshot; preparation edits stay separate until you load the updated program.\n\nUse blackout to hide audience content. Configure a default image for a holding screen and use Show image when needed; Resume returns to the current live slide.\n\nNDI requires a build with sender support and the NDI runtime. Transparent output leaves uncovered canvas areas transparent; media, text, and graphics remain visible. Test your receiver and physical displays before relying on them during a service."
        },
        {
            title: "Remote & saved data",
            body: "# Connect a remote\n\nUse the phone icon in the toolbar to open Remote. Select your local network interface and Start remote. Open the shown address or QR code on a device connected to the same trusted network, then use the displayed pairing details. Allow local network access if your operating system asks.\n\nA 127.0.0.1 address works only on this computer. Choose the Wi-Fi or Ethernet address for another device.\n\n## Keep your work\n\nSongs, imported Bibles, templates, programs, and preferences are stored locally. Imported Bibles remain usable offline. Use available export controls to keep copies of important work and media. Google Fonts must be downloaded before offline use.\n\nThe Selah rename preserves the original ChurchDeck / Church Deck storage namespace. Do not delete that folder while cleaning up old installations."
        }
    ]

    objectName: "userGuideDialog"
    title: "Selah user guide"
    modal: true
    parent: Overlay.overlay
    width: Math.min(940, parent.width - 48)
    height: Math.min(720, parent.height - 48)
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    padding: 20
    standardButtons: Dialog.Close

    background: Rectangle {
        color: LibraryStyle.panel
        border.color: LibraryStyle.border
        radius: 12
    }
    contentItem: RowLayout {
        spacing: 24

        ColumnLayout {
            Layout.preferredWidth: 205
            Layout.alignment: Qt.AlignTop
            spacing: 6

            Repeater {
                model: guide.topics

                LibraryButton {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    text: modelData.title
                    selected: guide.topicIndex === index
                    quiet: !selected

                    onClicked: {
                        guide.topicIndex = index;
                        readingArea.cursorPosition = 0;
                    }
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 1
            color: LibraryStyle.divider
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            TextArea {
                id: readingArea

                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.MarkdownText
                text: guide.topics[guide.topicIndex].body
                wrapMode: TextEdit.Wrap
                font.family: LibraryStyle.fontFamily
                font.pixelSize: 14
                color: LibraryStyle.text
                background: null
                padding: 0
            }
        }
    }
}
