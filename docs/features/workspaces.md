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
