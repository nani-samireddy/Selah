# Programs (service decks)

Click **Programs** in the top bar. A program is a saved service order containing independent snapshots of songs, Bible passages, custom presentations, images and videos. Preparing or selecting an item never automatically changes the live output.

## Prepare a service

1. Choose **New program**, enter a title and optional `YYYY-MM-DD` date, and create it. Programs first shows a searchable grid of saved program cards. Click a card to open its service editor; **All programs** returns to the grid. The last opened program remains available as the default library destination.
2. Choose **Add item** and select a song, Bible passage, image/video, the complete current editor presentation, or a `.churchdeck` presentation file. The song/Bible/media libraries show **Add to program** when opened from here.
3. Drag an item’s grip to reorder it, or use **Up**, **Down** and **Remove**. A drop commits one undoable move; Escape or dropping outside the list cancels. The list scrolls when dragging near its top/bottom edge. Removal and changes support Undo/Redo. Repeated additions are independent items.
4. Edit the item title or private speaker notes. These save after a brief pause, when switching items, and when closing Programs. Use **Edit details** in the editor header to change the program name and date. Confirming the dialog saves both together. Changes to the program model autosave to SQLite after 450 ms; **Save program** or Cmd/Ctrl+S saves immediately.
5. For a song, choose one of its saved arrangements. For a song or Bible passage, choose **Original**, **Midnight**, or **Warm paper** styling. These are built-in palette styles. **Choose template** previews and applies reusable content layouts; see [templates](templates.md).
6. Use **Edit slides** to open an item in the presentation editor. This is an undoable replacement of the editor draft. Use **Update program item** in Design, or return to Programs and choose **Update from editor**, to explicitly replace that item's slides. Live content remains unchanged. Choosing another song arrangement regenerates its slides from the stored lyrics, replacing manual slide edits.

Songs, passages and applied template definitions/revisions are stored with the program. Renaming or editing their library originals later cannot silently alter a prepared service. Adding songs preserves the chosen arrangement and language pairing. Programs do not currently offer automatic refresh from library sources.

## Present

- Select an item and a slide thumbnail, then choose **Present from here**. This explicit action starts live content; selecting or double-clicking preparation thumbnails does not publish them.
- The live snapshot contains the whole service. **Next** and **Previous** move across item boundaries. The service order marks the live item independently of the selected draft item.
- Use Right Arrow / Space for Next, Left Arrow for Previous, and Home / End for first/last slide in the Present workspace. Navigation shortcuts are disabled while entering text, using a selector, or browsing a content picker. Cmd/Ctrl+Z and the platform redo shortcut affect program history when focus is outside text fields.
- Use **Present from here** to enter Present with the prepared service. Current/next previews, live service navigation, audience blackout and audience/stage previews are available there. Outputs and Remote remain available globally.
- **Present from here** updates the snapshot. Reordering, changing an arrangement, editing notes or removing items never updates a running snapshot automatically.
- Audience and stage rendering, physical screen assignments and the NDI capture prototype use the existing OutputManager. Item notes are prepended to individual slide notes in the speaker view. NDI network transmission still requires the official SDK/runtime and receiver validation.
- In Design, **Present slide** explicitly switches the live source back to the editor, showing that editor snapshot in Present until a program is explicitly taken live again.

## Storage and exchange

Programs are stored in `programs.sqlite` in the same app data folder as the libraries. SQLite writes the program and last-opened preference in one transaction. Undo history is kept in memory (up to 40 changes); program selection and undo history are not restored across restarts. Up to 200 service items / 2,000 slides are supported per program, with at most 500 slides per item.

**Export** writes an atomic `.churchprogram` JSON file with format `churchdeck-program-1`. It includes item order, lyrics snapshots, references, arrangements, notes, styles and slide content. Import validates the document and creates independent program/item identities; it never overwrites a saved program. Invalid imports leave the current service in place.

Media remains referenced from the local managed media folder. Program export does not yet package media for transfer to another computer. Missing files are listed in Programs. Song/Bible content snapshots can still be presented without their source library records.

## Verification

```sh
ctest --test-dir build --output-on-failure
cmake --build build --target church-deck_qmllint
build/Selah.app/Contents/MacOS/Selah --fresh --program-smoke "$PWD/captures/program"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --program-smoke "$PWD/captures/program-compact"
```

Model checks cover create/save/reopen, independent export/import, arrangements, styles, title/notes, reorder/remove/undo/redo, missing media, opening/updating editor slides, rejecting invalid content without data loss, and live snapshot isolation. Rendering checks cover a mixed service with bilingual lyrics, Telugu Bible passages, media, current/next previews and separate audience/stage windows. Smoke runs use a separate verification profile and original test lyrics, preserving the normal user's programs and editor recovery file.

Physical three-display/hot-plug validation remains outstanding. Portable media bundles, whole-program duplication and library synchronization are not included in this implementation.

`--fresh --program-browser-smoke captures/program-browser` verifies the cards-first workflow, card navigation, a single editor heading, title/date edits, search, program creation and live-snapshot isolation using a temporary program database.

## Service sections

Assign service items to Worship, Sermon or Closing from the Section control above
Service order. Add custom sections with +, rename the selected section, or remove
its heading while keeping all items. Assignment groups items in section order;
items with no section appear first. New items inherit the selected section.
Moving an item across a section boundary assigns it to that destination section.
Undo/Redo includes these changes. Headings also appear in the loaded Present rail.

Sections organize existing content: they add no audience slides and trigger no
service cues. Editing sections changes the prepared program only; load/present
explicitly to replace an already loaded live snapshot. Section metadata travels
with program export, portable bundles and full backups.
