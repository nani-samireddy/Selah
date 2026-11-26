.pragma library

// Routes only reveal controls. Walkthroughs never create content or enable output.
var topics = [
    {id: "first-service", title: "Your first service", file: "getting-started.md", summary: "Prepare, arrange and present a service.", steps: [
        {title: "Start with a program", body: "Programs keep the songs, scripture, media and presentations for one service together. Open a saved program card or use New program when you are ready.", workspace: "programs", target: "workspaceNav_programs"},
        {title: "Build your content library", body: "Songs can be reused across programs. Quick Song accepts a title, lyrics and tags; blank lines create sections. You can try the controls or continue the tour.", library: "songs", target: "quickSongAction"},
        {title: "Add scripture", body: "Choose a Bible and book, then a chapter. The verse grid jumps to a verse; the verse cards select what you will add to the program.", library: "bible", target: "bibleBookPicker"},
        {title: "Choose a reusable layout", body: "Templates control the appearance of generated songs and scripture. Open a template card to edit its single canvas. Presentations are separate, multi-slide content.", library: "templates", target: "templateSearch"},
        {title: "Load and review", body: "Select your program here. Load program prepares its slides. When audience output is already enabled, the button says Take program live and changes what the audience sees. The tour does not press it.", workspace: "present", target: "presentProgramSelector"},
        {title: "Assign your displays", body: "Choose the audience and speaker displays, then review the selected monitor before enabling fullscreen. This walkthrough leaves all outputs unchanged.", outputs: "displays", target: "outputDisplaySettings"}
    ]},
    {id: "songs", title: "Songs & sections", file: "songs.md", summary: "Quick Song, languages, sections and arrangements.", steps: [
        {title: "Paste lyrics quickly", body: "Quick Song creates sections from blank lines. Give the song a name and tags, then review the sections before saving.", library: "songs", target: "quickSongAction"},
        {title: "Find songs by theme", body: "Use the tag filter and search to find worship themes, seasons or languages. Choose a song to edit all its sections as cards.", library: "songs", target: "songTagFilter"},
        {title: "Add the right arrangement", body: "After saving, choose the song order, layout and line limit. Add to program uses your chosen destination; its arrow lets you select another program.", library: "songs", target: "workspaceNav_libraries"}
    ]},
    {id: "bible", title: "Bible & dual Bibles", file: "bible.md", summary: "Find passages and show two translations together.", steps: [
        {title: "Choose your primary Bible", body: "Select an imported translation. Import translation offers the GitHub catalog and local files; downloaded Bibles work offline.", library: "bible", target: "bibleVersionPicker"},
        {title: "Find a passage", body: "Choose a book, then use the chapter and verse grids. Selecting a book opens chapter 1. Click the verse cards you want to include.", library: "bible", target: "bibleBookPicker"},
        {title: "Pair a second Bible", body: "Select a second translation to show the same reference in two languages. Choose a dual-Bible layout and review both texts before adding them to a program.", library: "bible", target: "bibleSecondaryPicker"}
    ]},
    {id: "design", title: "Templates & presentations", file: "design.md", summary: "Layouts, fonts, colors, undo and canvas controls.", steps: [
        {title: "Templates are reusable layouts", body: "Search the template library and open a card to edit its single canvas. Applying a template to a program keeps a snapshot, so later template edits do not change a prepared service automatically.", library: "templates", target: "templateSearch"},
        {title: "Presentations contain your slides", body: "Use New presentation for announcements, teaching and other custom content. Presentations can contain multiple slides and are separate from templates.", library: "custom", target: "newCustomPresentation"},
        {title: "Move around the canvas", body: "Pinch to zoom, use two-finger scrolling to pan, or drag with the Hand tool or middle mouse button. Click the zoom percentage to fit. Minimize Slides to keep a compact thumbnail strip beside the toolbar.", workspace: "design", target: "editorFloatingDock"},
        {title: "Edit and undo", body: "Select a layer to edit its properties. Double-click text to edit words. Undo and Redo live in the top bar; use Cmd+Z on macOS or Ctrl+Z on Windows. Background settings appear when you select empty canvas.", workspace: "design", target: "editorUndoButton"}
    ]},
    {id: "present", title: "Displays & presenting", file: "presenting.md", summary: "Programs, displays, transitions and live output.", steps: [
        {title: "Choose your program", body: "Select a program, then deliberately load it. Changing the dropdown alone leaves the audience unchanged. Take program live replaces content when output is active.", workspace: "present", target: "presentProgramSelector"},
        {title: "Review output routing", body: "Choose the audience and stage displays. Use Preview to inspect a window before fullscreen. NDI and remote settings are available in Connections below the displays.", outputs: "displays", target: "outputDisplaySettings"},
        {title: "Use one live toolbar", body: "Previous/Next, media, audio, Clear text and Blackout stay here. These controls affect live output. This tour highlights them without activating them.", workspace: "present", target: "presentTransport"},
        {title: "Handle unexpected content", body: "Quick Add previews songs, verses, messages and guest media while the current slide stays live. Present now interrupts with a return point; Insert next queues after the current item. Use Resume program to return. The walkthrough does not present anything.", workspace: "present", target: "quickAddOpenButton"},
        {title: "Stop presenting clearly", body: "Stop presenting turns off audience, stage and NDI output together, and pauses media and the countdown. It keeps your prepared content and position for later.", workspace: "present", target: "stopPresentingButton"}
    ]},
    {id: "remote", title: "Remote & saved data", file: "remote-and-data.md", summary: "Connect a phone and keep your content available offline.", steps: [
        {title: "Find remote control", body: "Use the phone icon to open Remote. Choose your Wi-Fi or Ethernet interface and start the server only when you are ready. The tour does not start it.", workspace: "present", target: "remoteToolbarButton"},
        {title: "Pair on your local network", body: "Open the shown address or QR code on a phone on the same trusted network. A 127.0.0.1 address works only on this computer; use the network address for another device.", outputs: "displays", target: "presentOutputControls"},
        {title: "Keep offline copies", body: "Libraries, programs and preferences are stored on this computer. Imported Bibles and downloaded fonts work offline. Use export controls to back up important content and media.", library: "media", target: "workspaceNav_libraries"}
    ]}
];
