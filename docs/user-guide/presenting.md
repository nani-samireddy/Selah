# Run the service

Select **Outputs** to jump to the display controls on the Present page and assign connected audience and speaker displays. Confirm the selected monitor before enabling output. The speaker view is for the operator or presenter and can include notes and upcoming content.

In Present, load a program, choose a service item, and select a slide. The live output uses the loaded snapshot; preparation edits stay separate until you load the updated program.

## Add unexpected content

Open **Quick Add** in Present, or press **⌘K** on macOS / **Ctrl+K** on Windows.
The current slide stays live while you search and preview.

- **Songs:** search saved titles or lyrics, including close spelling matches.
- **Bible:** choose a translation (and optionally a second Bible), then enter a
  reference or verse range. Book-name typos are matched against installed books.
- **Quick Song / Text:** paste lyrics or a message. Blank lines divide sections;
  long sections split into slides. Your draft is kept on this computer, and
  **Save to library** stores it as a song or presentation when you choose.
- **Media / Presentations:** choose saved content or import a local image, video, audio,
  or Selah presentation file. Other presentation formats must first be converted.

Review the slide preview, then choose **Present now** to interrupt immediately or
**Insert next** to queue after the current service item. Repeated queued additions
keep their insertion order. Quick items are labeled in the service order and
belong to the loaded session; they do not rewrite the saved program.

**Resume program** returns to the interrupted slide after Present now. When a
queued addition is reached normally, Resume skips to the next program item.
Loading another program replaces the session queue and its return point.
Quick Add works without AI or internet access. It uses the current display routes,
and preserves blackout and clear-text state.

Use the red **Stop presenting** button at the right of Present’s pinned bottom
toolbar to stop audience, stage and NDI outputs together. It also pauses media and
the speaker countdown. Your prepared program and slide position remain available.
The same action is under **Presentation → Stop presenting**.

Use blackout to hide audience content. Configure a standby media for a holding screen and use Show image when needed; Resume returns to the current live slide.

NDI requires a build with sender support and the NDI runtime. Transparent output leaves uncovered canvas areas transparent; media, text, and graphics remain visible. Test your receiver and physical displays before relying on them during a service.

## Output controls

In **Present**, scroll below the audience and next-slide previews. Use **Displays** to assign audience and speaker screens or open NDI
and remote settings. Navigation, media playback, audio, text visibility, blackout,
and Stop presenting stay in the pinned bottom toolbar. **Speaker tools** contains
speaker notes, the speaker message, stage preview and countdown.
Audience blackout stays in the pinned Present transport bar on every control tab. The operator screen supports
a windowed Preview; choose another screen for fullscreen.

Slides are grouped by service item in the left sidebar. Expand a song, scripture,
or presentation to choose an individual slide. Moving to another item opens its
slide group automatically. Preparing a snapshot shows **Ready**; an enabled audience
output or NDI sender shows **Live**, with a green toolbar and footer.
Blackout changes the live cue to amber. Diagnostic capture alone does not show Live.

## Slide transitions

In **Design**, use **Presentation default** in the inspector to choose Cut, Fade, Fade through black/white, directional Wipe/Slide/Swipe,
Zoom in/out, Spin, Blur, Iris in/out, or Paint brush.
Effect duration is in milliseconds (50–5000). **This slide** can override that default;
choose Use default to inherit it again. New slides inherit the presentation default.
Templates remain single-canvas layouts and do not have transition controls.

In **Programs**, use the selected item's **Transition** control to apply an effect
to all its slides, including generated songs and Bible passages. **As authored**
preserves the saved presentation's per-slide settings. Changes remain in preparation
until you present the updated program.

Audience displays and NDI share one transition clock. Uncovered areas stay transparent in
NDI overlay output. Blackout, clear-text and standby actions apply immediately. Rapid
navigation during a fade cuts to your latest selected slide instead of queuing fades.
Older presentations keep Cut until you choose Fade. UI reduced-motion preferences
are separate from these authored presentation effects. Software graphics use Cut.

## Follow a Scripture reading

Open **Scripture** in Present for an open-ended reading. Choose a Bible, an optional second Bible and a template; Selah remembers these choices when you close the panel. Search a reference or use the book, chapter and verse grids. Select one verse or several verses and check the preview. Browsing does not change the live output. In Scripture mode, use **↑ / ←** for the previous verse and **↓ / →** for the next verse. Navigation follows the displayed search results, or continues across chapters and books when browsing a chapter. Arrow keys retain their normal behavior while editing a field or choosing a dropdown.

**Present verse** or **Present passage** takes the selection live and retains it under Scripture reading in the live deck. **Next verse** and **Previous** present the adjacent verse, crossing chapter and book boundaries. If a passage splits across slides, these buttons move through its slides first. You can jump to another reference at any time. Presenting the same selection with the same layout reuses its existing entry.

**Resume program** returns to your original program slide. Closing the Scripture panel keeps the current slide live. Additions belong to this live session and do not modify your saved program. Audience output is controlled separately using the display controls.

## Service cues

The cue buttons near the top of Present run saved action sequences. Open their settings, choose **New cue**, name it, and use **Add action**. Configure each step and use the arrow icons to reorder it. You can combine stage/audience settings, navigation, countdowns, media, overlays, audio clips, transitions, waits, and HTTP/OSC commands.

**Test** validates the setup without executing anything or sending network commands. **Run cue** executes the current editor contents; **Save cue** stores it for the Present buttons. Only an explicit Start output/Preview action turns on a stopped screen. Fullscreen actions retain operator-screen protection.

Use **Cancel cue** to stop pending steps. Operator slide navigation and Stop presenting also cancel a running cue. Completed actions remain applied; a failed step stops later ones. Review and test new cues before a service.

## Recover an interrupted service

After an unexpected exit, Present offers **Restore session** or **Discard**. Restoration recovers your current slide, spontaneous Scripture/Quick Add groups and original program resume point. Outputs remain off, audio is muted, and media/countdown are paused until you deliberately resume. A normal quit or **Stop presenting** clears this recovery checkpoint.

## Audio files

Import MP3, WAV, M4A/AAC, FLAC, OGG, AIFF or Opus through Libraries → Media. Use the **Audio** filter to browse them. Click an audio card to preview it with play/pause, seeking, position/duration and mute controls. Preview starts muted; choose **Unmute** to listen. Closing the preview stops it.

Audio can be added to a program or slide and queued through Quick Add. It plays once by default; select its layer in Design to enable Loop or mute that layer. In Present, use the existing media play/pause and audio controls. Live audio plays on the computer's default audio device independently of projector windows. A single player set serves the live session, so opening previews or NDI does not double its sound. NDI audio transmission is not added by this feature.

Audio layers do not draw on audience output. An audio-only program item shows the configured standby media when available, otherwise its canvas background. Changing live slides stops the previous item's audio; **Stop presenting** pauses it. Recovery leaves playback paused and audio off. Program bundles and full backups include audio files. Codec availability can vary by platform.

## Output button states

Audience and Stage preview buttons change to **Close preview** while their window is open. Click again to close it; closing the window itself also resets every corresponding preview button. The editor's slide-preview icon follows the same open/close behavior.

Fullscreen buttons show **Stop fullscreen** on the current display. If you select a different display while fullscreen is active, the action becomes **Move fullscreen**. Display cards identify Preview, Fullscreen, Blackout or Stopped. Media, audio, text visibility, and blackout controls show their existing active state.

## Standby media

In Present, use **Standby media → Choose media** to select an image or video. Importing copies the file into Selah without changing live content. Select **Show standby media** when ready, and **Resume presentation** to return to the same program slide and playback state.

Videos loop silently, even when the presentation behind them is paused. Fit/Fill and changing the file apply the next time you show standby media. The preview tile displays the selected image/video; decoder errors appear there. Images support up to 50 MB / 50 megapixels, and videos up to 2 GB. Supported video containers are MP4, MOV, M4V, WebM, MKV and AVI; codec availability depends on the platform. Existing default-image preferences are preserved.
