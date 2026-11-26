# Testing and diagnostics

Configure the normal development build with `BUILD_TESTING=ON` and `CHURCHDECK_ENABLE_DIAGNOSTICS=ON`. Defaults are enabled. The `release` preset disables both.

```sh
cmake --build build
python3 scripts/format.py --check
cmake --build build --target church-deck_qmllint
ctest --test-dir build --output-on-failure
```

`tests/church-deck-tests` is a standalone model runner. It covers draft editing/round-trips, content imports/search, shaping/splitting, media catalog state, service snapshots/persistence live routing state and loopback HTTP remote sessions/commands. Fixtures use temporary databases and Qt's test data profile. Test definitions are stored in `tests/unit`, separate from runtime implementation files. New cases should check behavior and data safety rather than mirror an implementation.

## Visual smoke checks

The following macOS executable paths assume the development build. On Windows, use `build/Selah.exe` instead. Create the capture folder before using standalone screenshot mode.

```sh
mkdir -p captures
build/Selah.app/Contents/MacOS/Selah --fresh --screenshot "$PWD/captures/editor.png"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --template-smoke "$PWD/captures/templates"
build/Selah.app/Contents/MacOS/Selah --fresh --workspace-smoke "$PWD/captures/workspaces"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --workspace-smoke "$PWD/captures/workspaces-compact"
build/Selah.app/Contents/MacOS/Selah --fresh --content-smoke "$PWD/captures/content"
build/Selah.app/Contents/MacOS/Selah --fresh --program-smoke "$PWD/captures/program"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --program-smoke "$PWD/captures/program-compact"
build/Selah.app/Contents/MacOS/Selah --fresh --routing-smoke "$PWD/captures/routing"
./scripts/check-media.sh
./scripts/check-ndi.sh
build/Selah.app/Contents/MacOS/Selah --fresh --remote-demo "$PWD/captures/remote"
```

The remote demo stays running for browser integration checks; close it when finished. It writes test-only pairing data and binds loopback, never a public interface. See the [remote guide](features/remote-control.md).

Media/NDI helper scripts generate fixtures with FFmpeg; FFmpeg is not an application dependency. `--physical-smoke <folder>` routes real output to a connected non-operator display and should only be run when those displays are available for testing. Physical unplug/reconnect and three-display operation are still unverified.

Profiles are selected before constructing models or reading settings. `--fresh` skips recovery within that run's selected profile; it is not a request to erase normal data. Screenshot mode uses a capture profile; other diagnostics use a verification profile. Run visual scenarios sequentially so separate windows and test-profile writes cannot interfere with each other.

Inspect the process exit status and log, then view the generated images/reports. Check text shaping, clipping, live/draft isolation, current/next contents and stage-only notes. SDK-free NDI checks verify capture/alpha/queuing only; they do not prove network transmission or audio interoperability.

## Runtime-only build

```sh
cmake --preset release -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build --preset release
```

This build excludes model-test bodies and smoke commands. Verify this configuration after changing build boundaries. For self-contained macOS packages, use `python3 scripts/package-macos.py`. The helper audits dependencies, verifies ad-hoc signatures, checks DMG integrity and records checksums. Developer ID notarization and Windows deployment remain separate release work.

To render every bundled Meadow/Garden template and check actual QML text bounds and media errors:

```sh
build/Selah.app/Contents/MacOS/Selah --fresh --template-presets-smoke "$PWD/captures/template-presets"
```

This uses an isolated verification profile and a temporary template catalog. It writes individual
PNGs and a labeled contact sheet. It does not take slides live or alter the normal editor/programs.

Present-specific native checks (isolated profile, windowed preview and NDI diagnostic capture):

```sh
build/Selah.app/Contents/MacOS/Selah --fresh --present-smoke "$PWD/captures/present-workspace"
```

This captures empty, compact dark, standby and wide light states, verifies the default image on
both the audience and NDI rendering paths, checks blackout priority, and resumes the same program
snapshot/slide. It uses a diagnostic NDI sink and does not send video over the network.

Bible XML import checks include Beblia/Zefania parsing, partial/NT-only numbering, omitted verses,
Unicode search, metadata preservation, duplicate rejection and atomic rollback. To check the actual
Telugu BSI repository file in a disposable catalog:

```sh
build/tests/church-deck-tests --bible-xml-file /path/to/TeluguBible.xml
```

For the complete native picker → download → index → select translation workflow (network required):

```sh
build/Selah.app/Contents/MacOS/Selah --fresh --bible-import-smoke "$PWD/captures/bible-import"
```

That run uses an ephemeral scripture database and a verification profile, captures the repository
picker and imported chapter, and checks that the editor/live snapshot remain unchanged.

Dual Bible regression coverage lives in `tests/unit/DualScriptureTests.cpp`:
canonical reference matching, missing/same-translation rejection, incompatible
layouts, lossless Indic pagination, attribution and program save/reopen isolation.
`--fresh --dual-scripture-smoke captures/dual-scripture` exercises the native QML
translation picker, verse selection, paired preview, and offline guide dialog in
a separate verification profile; it captures screenshots without changing saved
operator programs.

`--fresh --output-ux-smoke captures/output-ux` checks output settings at the minimum
window size: section visibility, operator-screen protection, role-specific blackout,
pinned transport blackout, navigation/audio/text controls, speaker message/countdown, and
snapshot/draft isolation. It uses a separate verification profile, never starts
fullscreen or preview outputs, and captures each section for layout review.

`--fresh --workspace-smoke captures/ux-feedback` also checks template search,
grouped service items, selected-word font previews, single-canvas template editing,
and original-resolution media previews with Fit/100% views. Add
`--preview-video /absolute/path/to/video.mp4` to exercise original-video playback
and release-on-close. `--present-smoke` verifies Ready/Live/Blackout ambience, including
that NDI diagnostics alone do not indicate an active sender.

`--fresh --transition-smoke captures/transitions` checks the actual GPU-rendered
fade endpoints and midpoint, physical-audience/NDI timing, premultiplied overlay
alpha, rapid navigation and immediate blackout. Add
`--transition-video /absolute/path/to/video.mp4` to verify that a shared video
layer keeps decoding through slide changes without replacing its delegate.
`tests/unit/TransitionTests.cpp` covers defaults/overrides, validation, persistence,
undo and isolation of live snapshots from program preparation edits.

Present diagnostics also click Stop presenting with audience, stage and NDI capture
active. They check that all outputs stop, media pauses, and the snapshot/index remain
unchanged, then exercise Stop during NDI startup to reject a queued sender-ready signal.

`--fresh --resize-smoke captures/editor-resize` sends actual pointer drags through
all four text/image corner handles, including zoom, rotation, an active rich-text
editor and overlapping layer hit areas. It checks transient preview vs committed
geometry, fixed opposite corners, one-step undo, snapping, canvas/minimum bounds,
locked layers and cancellation when switching tools. The live snapshot stays
unchanged throughout.

`--fresh --background-smoke captures/canvas-background` checks the template editor's
Background action, color field and Color/Transparent switch, saving/composing the
template, reversible color restoration, and actual GPU capture with zero/partial
alpha. Authored artwork remains opaque, ambient fill shares the canvas alpha, and
checkerboards do not leak into output. Template model checks cover alpha persistence,
composition, import/export and undo.

## Cross-platform portable-bundle exchange

The Mac-generated fixture in `tests/fixtures/macos-service.selahbundle` contains
original bilingual content, a template snapshot, service sections and managed image
bytes. From a native standalone test runner:

```sh
build/tests/church-deck-tests --bundle-exchange-import tests/fixtures/macos-service.selahbundle captures/bundle-exchange
```

On Windows use `build/tests/church-deck-tests.exe`. The check imports offline into
a temporary profile, checks relocated image pixels/content/sections and re-exports
`<platform>-roundtrip.selahbundle` for the other OS to verify. It never starts outputs.
Use `--bundle-exchange-export <file>` to regenerate the original fixture intentionally
on macOS and record that provenance. This does not replace actual service/codec/font
or backup migration rehearsals on installed packages.
