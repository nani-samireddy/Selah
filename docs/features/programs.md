# Programs (service decks)

Click **Programs** in the top bar. A program is a saved service order containing independent snapshots of songs, Bible passages, custom presentations, images and videos. Preparing or selecting an item never automatically changes the live output.

## Prepare a service

1. Choose **New program**, enter a title and optional `YYYY-MM-DD` date, and create it. The program selector reopens saved services. The last opened program is restored on launch.
2. Choose **Add item** and select a song, Bible passage, image/video, the complete current editor presentation, or a `.churchdeck` presentation file. The song/Bible/media libraries show **Add to program** when opened from here.
3. Select an item and use **Up**, **Down** or **Remove** to curate the order. Removal and changes support Undo/Redo. Repeated additions are independent items.
4. Edit the item title or private speaker notes. These save after a brief pause, when switching items, and when closing Programs. Program title/date changes use **Apply details**. Changes to the program model autosave to SQLite after 450 ms; **Save program** or Cmd/Ctrl+S saves immediately.
5. For a song, choose one of its saved arrangements. For a song or Bible passage, choose **Original**, **Midnight**, or **Warm paper** styling. These are built-in palette styles. **Choose template** previews and applies reusable content layouts; see [templates](templates.md).
6. Use **Edit slides** to open an item in the presentation editor. This is an undoable replacement of the editor draft. Return to Programs and choose **Update from editor** to explicitly replace that item's slides. Live content remains unchanged. Choosing another song arrangement regenerates its slides from the stored lyrics, replacing manual slide edits.

Songs, passages and applied template definitions/revisions are stored with the program. Renaming or editing their library originals later cannot silently alter a prepared service. Adding songs preserves the chosen arrangement and language pairing. Programs do not currently offer automatic refresh from library sources.

## Present

- Select an item and a slide thumbnail, then choose **Take slide live**. Double-clicking a thumbnail also takes it live.
- The live snapshot contains the whole service. **Next** and **Previous** move across item boundaries. The service order marks the live item independently of the selected draft item.
- Use Right Arrow / Space for Next, Left Arrow for Previous, and Home / End for first/last slide while Programs is open. Navigation shortcuts are disabled while entering text, using a selector, or browsing a content picker. Cmd/Ctrl+Z and the platform redo shortcut affect program history when focus is outside text fields.
- Current/next previews, audience blackout, audience/stage windowed previews and output settings are available beside the service order. Scroll that column on smaller windows.
- **Take slide live** updates the snapshot. Reordering, changing an arrangement, editing notes or removing items never updates a running snapshot automatically.
- Audience and stage rendering, physical screen assignments and the NDI capture prototype use the existing OutputManager. Item notes are prepended to individual slide notes in the speaker view. NDI network transmission still requires the official SDK/runtime and receiver validation.
- In Output settings, **Take editor slide live** explicitly switches the live source back to the editor, disabling program navigation until a program slide is taken live again.

## Storage and exchange

Programs are stored in `programs.sqlite` in the same app data folder as the libraries. SQLite writes the program and last-opened preference in one transaction. Undo history is kept in memory (up to 40 changes); program selection and undo history are not restored across restarts. Up to 200 service items / 2,000 slides are supported per program, with at most 500 slides per item.

**Export** writes an atomic `.churchprogram` JSON file with format `churchdeck-program-1`. It includes item order, lyrics snapshots, references, arrangements, notes, styles and slide content. Import validates the document and creates independent program/item identities; it never overwrites a saved program. Invalid imports leave the current service in place.

Media remains referenced from the local managed media folder. Program export does not yet package media for transfer to another computer. Missing files are listed in Programs. Song/Bible content snapshots can still be presented without their source library records.

## Verification

```sh
ctest --test-dir build --output-on-failure
cmake --build build --target church-deck_qmllint
build/church-deck.app/Contents/MacOS/church-deck --fresh --program-smoke "$PWD/captures/program"
build/church-deck.app/Contents/MacOS/church-deck --fresh --compact --program-smoke "$PWD/captures/program-compact"
```

Model checks cover create/save/reopen, independent export/import, arrangements, styles, title/notes, reorder/remove/undo/redo, missing media, opening/updating editor slides, rejecting invalid content without data loss, and live snapshot isolation. Rendering checks cover a mixed service with bilingual lyrics, Telugu Bible passages, media, current/next previews and separate audience/stage windows. Smoke runs use a separate verification profile and original test lyrics, preserving the normal user's programs and editor recovery file.

Windows packaging and physical three-display/hot-plug validation remain outstanding. Portable media bundles, drag-to-reorder, whole-program duplication and library synchronization are not included in this implementation.
