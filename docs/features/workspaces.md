# Application workspaces

Selah has four persistent workspaces in the top navigation. The app opens in Programs. Outputs and Remote remain available from every workspace; these settings use focused dialogs, while the primary workspaces occupy the application body.

## Programs: prepare the service

Choose or create a program, add songs/passages/media/presentations, arrange items, preview slides and edit notes/styles. Selecting an item or slide prepares it without changing live output. **Present from here** explicitly takes the complete prepared program live from the selected slide and opens Present. Double-clicking preparation thumbnails no longer publishes them.

**Edit slides** opens the item in Design. After editing, **Update program item** copies the design back to the selected item and returns to Programs. That action is available only when the selected program item matches the item being edited. Program draft changes remain separate from a running service.

## Libraries: browse and prepare content

Songs, Bible, Media and Templates have persistent tabs. Searches, selections and song drafts remain in their existing panels when switching workspaces. The **Add content to** selector makes the destination explicit: the current program or the Design presentation. When no program exists, the destination is Design.

Adding content returns to its destination and shows a confirmation. Saving a song updates the library and keeps the user in place. Template editing opens Design. Program **Add item** and Design media insertion still use contextual pickers; they do not change the global workspace underneath.

## Design: edit the presentation

The floating canvas, layers, inspector and tool dock preserve the established Modfy/Toolcraft visual direction. The contextual toolbar contains Open, Export, Preview and **Present slide** or **Save template**. Program item designs have a direct **Update program item** action.

Preview remains a local preview. Present slide explicitly copies the editor presentation to the live state and opens Present. Editor shortcuts apply only in Design with no interfering dialog.

## Present: operate the running service

A live slide list, larger current/next still previews, speaker notes/message/countdown and a persistent transport row share one screen. Previous/next, media playback, clear text and audience blackout act on the live output. Arrow/Space/Home/End navigation is active here and disabled while settings dialogs are open.

The list is derived from `OutputManager.liveOrder`, a lightweight snapshot-only projection. It does not follow the current preparation selection. Navigation updates its current row without rebuilding the snapshot list. A global live/blackout indicator stays visible while preparing content elsewhere. Current/next cards are still previews; video and audio continue on the actual output windows.

## Implementation and checks

`qml/workspaces` contains the navigation, context bars and Present components. `Main.qml` owns transitions and connects content actions to their destinations. Existing panels support a nonmodal full-workspace mode and retain modal picker mode, so content editor behavior is shared rather than duplicated. Program item edits flush before switching workspaces; draft/live boundaries stay in the C++ models.

Use the isolated diagnostic to capture every workspace and check browsing/live isolation:

```sh
build/Selah.app/Contents/MacOS/Selah --fresh --workspace-smoke "$PWD/captures/workspaces"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --workspace-smoke "$PWD/captures/workspaces-compact"
```

`--workspace-demo <folder>` runs the same checks and stays open for native keyboard/mouse verification. These commands use a dedicated UX verification data profile and original lyrics. Existing feature diagnostics continue to start in Design so their canvas setup remains meaningful.

Physical operator rehearsal and keyboard/IME validation on Windows remain required before release.

## Present workspace and default image

Present keeps the live service list on the left, an audience preview
in the center, and Up Next and a default-image card on the right. The bottom transport
keeps Previous/Next, media pause, Clear text, Blackout and output settings in fixed positions.
The audience preview reflects cleared text, blackout and standby; Up Next continues to show the
next prepared program slide. Compact and wide layouts use the same controls in both appearances.

Choose a church logo or welcome image in **Present → Default image → Choose image**. Selah copies
the file into the profile's `default-images` directory and saves the preference, so deleting the
original download does not break it. Fit preserves the whole image with black letterboxing; Fill
crops to fill the 16:9 canvas. Choosing/replacing an image or changing its fit only prepares a
preference. It does not change live output. Images must decode successfully and be under 50 MB
and 50 megapixels.

**Show default image** temporarily replaces audience/NDI content. It also works before a program
is live. **Resume presentation** returns to the same slide, retaining the program snapshot,
selection, speaker notes, countdown, media play/pause setting and independent blackout/text-clear
states. A still default image is not an overlay on top of playing media; video layers reload when
the presentation is restored. Taking a slide live or navigating to a valid slide exits standby.
Blackout always takes priority. Display routing remains explicit; showing an image does not start
an output window or NDI sender.

Stage continues to show the current program text/notes, with a default-image indicator. Remote
clients receive a standby status indicator; their slide navigation resumes the presentation.
The default image is never shown automatically at app launch or at the end of a program. Its
active state is temporary and only its selected file/name/fit are saved. Removing/changing the
preference while an image is live leaves that live image intact until hidden or replaced.

## Application menus

On macOS, File, Edit, View, Presentation and Help appear in the system menu bar
after Selah. Other desktop platforms show the menu bar inside the window.
File actions reuse the existing program/presentation dialogs. Save targets the
active Programs or Design workspace; it is disabled in other workspaces or while
a settings/dialog flow is open. Edit routes clipboard and history actions to a
focused text field, then to the active program or design document. View switches
workspaces and appearance; Presentation opens output, remote and NDI controls and
operates the current live snapshot. Help opens the same offline guide as the
book toolbar button (F1).

Menu actions own the shared keyboard shortcuts so the toolbar and workspace
panels do not register competing Save/Undo/Redo shortcuts. Native menu verification
on macOS uses `--fresh --menu-smoke captures/native-menu` in a diagnostics build;
it checks menu definitions and workspace/help actions in a separate verification
profile. AppKit system-bar assertions additionally run when macOS grants the test
window foreground activation; otherwise verify placement in the running app.

File → Import program and Export program use the same `.churchprogram` dialogs as the Programs toolbar. Pending item edits are flushed before either dialog opens; export requires an open program.

Programs opens to a searchable card grid. Each card opens a dedicated service editor with a single heading, service date and Edit details dialog. Use All programs to return to the grid. Returning after adding library content or updating a designed program item opens the current service editor directly.

The Present page contains output controls below the display previews, in two sections: Displays (audience/stage routing and network
connections), and Speaker tools (notes, message, stage preview and countdown).
Live transport actions have one home in the pinned bottom toolbar, including audio. Each display
card shows its own status. Operator-screen fullscreen is disabled; windowed preview
remains available. Audience blackout stays accessible in the pinned Present transport bar. Opening
or switching sections does not route displays or change the loaded snapshot.

The Present service list groups child slides by service item. Live ambience follows
active audience/NDI sending, rather than merely having a loaded snapshot. Template
editing uses one canvas; slide management belongs to Presentations. The template
library offers searchable preview cards that open directly in Design. The program
layout picker retains its content preview before applying a selected layout.
Media thumbnails are regenerated at up to 1280 × 720 without upscaling originals;
clicking a thumbnail opens the original with image Fit/100% or video playback.

Speaker notes and the stage countdown live under Speaker tools, rather than beneath
the audience preview. Live state uses a strong toolbar/footer tint and the LIVE
badge; there is no full-window outline.

The pinned Present transport includes a red Stop presenting action. The host stops
both display roles, cancels pending fullscreen routing, stops NDI (including startup),
pauses media/countdown and retains the prepared snapshot and selected slide. The
Presentation menu exposes the same action; it is disabled when no output is active.
