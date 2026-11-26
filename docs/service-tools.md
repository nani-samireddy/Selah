# Service tools

## Songs with chords

Open Libraries → Songs and enable **Chords & cues**. Type chords immediately before their lyric position, for example `[C]Amazing [G]grace`. The live preview places chords above the words. Enter an optional song key and a stage cue for each section (for example, “Repeat chorus” or “Instrumental”). These values save with the song and its program snapshot.

**Import song** accepts Selah JSON and ChordPro (`.cho`, `.chopro`, `.chordpro`, `.pro`). Supported directives include title, artist/subtitle, key, section starts/ends, and comments. Blank lines start new sections. Import one song per file. This is lyric-and-chord import; tablature, diagram directives, and automatic chord generation are not supported.

Under Present → Speaker tools choose **Musician**. The stage output shows chords with the current and next lyrics, section/key, and the current section’s cue. Audience output and audience NDI contain only lyrics. Chords are anchored to the shaped lyric text, including Telugu, rather than aligned with spaces. Wrapping and bilingual template pagination retain the associations. Transposition, capo settings and separate musician devices are future additions.

The song editor uses editable ChordPro tokens with a rendered preview. Click-to-place chord editing is a future improvement. If lyrics are subsequently changed directly in a slide, stale chord anchors are suppressed; regenerate the program item from the edited library song to update both together.

## Interrupted live sessions

Selah atomically checkpoints the loaded live deck, current slide, spontaneous additions (including Scripture and Quick Add groups), original program resume point, stage message, remaining countdown, and stage view. It uses the existing local media; recovery does not copy media files.

After an unexpected exit, Present offers **Restore session** or **Discard**. Nothing is restored before that choice. Restoration loads the snapshot with audience/stage outputs off, audio off, media paused, and countdown paused. Review the slide and start your outputs deliberately. A stopped presentation or normal application quit clears the recovery checkpoint. An unanswered recovery offer remains available on subsequent launches.

Recovery is local to this computer. Missing or moved media cannot be recreated from a session snapshot. NDI and remote services never auto-start on recovery.

## Portable program bundles

Open a program and choose **Export → Portable bundle with media…**, or File → **Export portable program…**. A `.selahbundle` includes the saved program, lyrics/Scripture snapshots, template snapshots, images, videos and image backgrounds referenced by that program. Only saved content is exported; loading it does not alter a currently live snapshot.

Use **Import program…** on the other computer. Import verifies media hashes, copies files to a new managed directory and creates a new program with fresh IDs. It does not overwrite an existing program or start outputs. The source media can be absent after export; the imported program runs offline. Original content libraries and Bible databases are not required to display the saved slides.

The format is versioned, streamed, and uncompressed. Import/export show progress and can be cancelled. Limits: 32 MB manifest, 20,000 media references, 50 GB total media. Missing media prevents a successful export. Damaged bundles are rejected before a program is saved, and partial extraction is removed.

Fonts are not redistributed. Install matching fonts on the destination and check the presentation before use. Global standby medias, output routing, and local service-cue presets are computer preferences and are not part of a program bundle.

## Ordered custom service cues

Open Present → Service cues → settings. Select a cue or choose **New cue**, give it a name, choose an action type, and select **Add action**. Each action has its own settings. Use the up/down icon buttons to reorder actions and the trash icon to remove a step. Save the cue to show its button in Present. Existing cues migrate to ordered actions without changing their names or settings.

Supported actions:

- Speaker/Musician stage view; audience lyrics, standby media, blackout, clear text or restore.
- Presentation audio on/off, media play/pause, countdown start/pause and stage messages.
- Navigate to a one-based slide number, a stable service-item identity in the loaded live deck, or the original program resume point.
- Set the live deck's transition effect/duration, or return to authored transitions.
- Show/clear a full-canvas image overlay on the audience composition, including audience NDI. PNG transparency is retained. The stage display is unchanged.
- Play/stop a local audio clip independently of presentation media. Clips use the system's default audio device at 80% volume.
- Explicitly start the configured audience/stage fullscreen output, open its windowed preview, or stop it. Fullscreen requires an available external display and retains operator-screen protection.
- Wait between actions (1–60,000 ms per step; two minutes of waits total).
- Send HTTP GET/POST requests (POST uses a JSON content type). Requests wait for a 2xx response; redirects are not followed and network inactivity times out after five seconds. Response bodies are discarded.
- Send an OSC UDP command to a numeric IP and port. Arguments are a JSON array of int32/float32, strings and booleans; for example `[1, "go", 0.5, true]`. UDP transmission does not confirm that the receiving device acted on the message.

**Test** validates settings and current local prerequisites without changing live state, playing clips, starting outputs or sending any network commands. It cannot confirm a remote service's response or decode every audio format. **Run cue** executes the current editor contents; a saved cue's Present button executes the saved version.

All known prerequisites are checked before the first action. An execution failure stops remaining steps; completed actions remain applied. **Cancel cue** cancels waits/network work and stops cue audio without undoing completed actions. Operator slide navigation, replacing the loaded deck, or Stop presenting cancels a running sequence. A cue cannot overlap another cue. Runs never auto-resume after recovery.

Cues support up to 32 steps and save locally. Full backups include their definitions and referenced local assets. Program bundles do not include global cues. Native MIDI, applying another content template, arbitrary scripts, and automatic slide-triggered cues are not implemented.

## Full app backups

File → **Backup & restore…** exports a `.selahbackup` with the managed app profile and preferences, plus referenced external media. SQLite databases use consistent snapshots even while open. Restore validates and stages files without replacing open databases, relocates paths for the destination profile, and applies only on the next launch. The previous profile and its preferences remain in a `.before-restore-…` sibling directory. Cancel restore before quitting to keep the current installation untouched.

Backups include downloaded fonts/models, but a runtime executable from one operating system may require reconfiguration on another. Matching system fonts still need installation on the destination. Backup limits are 20,000 files/references, 50 GB total assets and a 32 MB manifest. Partial downloads, database journals and lock files are omitted. Output controllers always start inactive.
