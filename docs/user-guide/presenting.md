# Run the service

Select **Outputs** to jump to the display controls on the Present page and assign connected audience and speaker displays. Confirm the selected monitor before enabling output. The speaker view is for the operator or presenter and can include notes and upcoming content.

In Present, load a program, choose a service item, and select a slide. The live output uses the loaded snapshot; preparation edits stay separate until you load the updated program.

Use the red **Stop presenting** button at the right of Present’s pinned bottom
toolbar to stop audience, stage and NDI outputs together. It also pauses media and
the speaker countdown. Your prepared program and slide position remain available.
The same action is under **Presentation → Stop presenting**.

Use blackout to hide audience content. Configure a default image for a holding screen and use Show image when needed; Resume returns to the current live slide.

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
