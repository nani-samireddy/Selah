# Selah

A native church presentation app built with **Qt Quick/QML and C++20**, targeting Windows and macOS. The current implementation is a working development prototype; it uses no Electron or embedded browser UI.

Prepare multilingual songs, Bible passages, media and custom slides, arrange them into a service program, and present through audience and speaker outputs. Editor drafts, prepared programs and live snapshots are separate states: editing a draft never silently changes a running output.

## Build and run

Requirements: CMake 3.24+, a C++20 compiler, Ninja (for the presets), and Qt 6.11+ with Quick, Quick Controls, Multimedia, Concurrent, Sql, Network and Gui private development headers. This Mac has been verified with Qt 6.12. Qt's private graphics API is used only by the NDI capture prototype and requires validation when upgrading Qt.

On macOS with Homebrew Qt installed:

```sh
./scripts/run.sh
```

Or configure explicitly:

```sh
cmake --preset dev -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build --preset dev
```

On macOS the executable is `build/Selah.app/Contents/MacOS/Selah`; on Windows it is `build/Selah.exe`. Use a Qt kit matching your compiler and architecture. On Windows, run from a developer terminal with the Qt runtime DLLs available. Building and deploying an installer on Windows is still to be validated.

Development builds use locally installed Qt libraries. The macOS packaging helper creates a self-contained, ad-hoc-signed app, ZIP and DMG; Developer ID signing and Apple notarization remain separate distribution steps.

## Use the app

The app opens in **Programs**. Use the persistent **Programs · Libraries · Design · Present** navigation to prepare services, browse content, edit slides and operate the live output. See the [workspace guide](docs/features/workspaces.md).

- **Libraries → Songs**: multilingual lyrics, paired translations, named sections/arrangements and slide generation.
- **Libraries → Bible**: three offline 66-book translations (English WEB, Telugu IRV and Hindi IRV), reference/phrase search and automatic splitting.
- **Libraries → Media**: managed image/video imports and reusable thumbnails.
- **Libraries → Templates**: reusable lyric, bilingual and scripture layouts with bound content and layout-driven splitting.
- **Programs**: curate a complete service; reorder items, set notes/styles, save/reopen and take a snapshot live.
- **Design → Font family**: search all 1,961 Google Fonts families, download on demand and reuse offline. See [fonts](docs/features/fonts.md).
- **Design**: change text/geometry, manage layers, pan/zoom, undo/redo and import/export `.churchdeck` presentations.
- **Outputs → Remote**: QR/code pairing for a phone/tablet controller or a separate read-only speaker view.
- **Outputs**: route audience/stage views, navigate live content, blackout independently and show speaker notes/timers/messages.

See the [documentation index](docs/README.md), [program guide](docs/features/programs.md), [content library guide](docs/features/content-libraries.md), [template guide](docs/features/templates.md) and [UI design contract](docs/ui-design.md). Original sample songs and the user-supplied Telugu song are available as importable JSON under [examples/songs](examples/songs).

## Repository map

```text
src/
  app/          Application startup, profiles and bundled fonts
  document/     Editable slides, history, validation, persistence, media insertion
  content/      Songs, Bible queries/import and slide generation
  media/        Persistent asset catalog and thumbnails
  programs/     Curated services, snapshots and persistence
  templates/    Reusable layouts, shaped text fitting and population
  outputs/      Display routing and live presentation state
  broadcast/    Offscreen rendering and bounded NDI sender worker
  remote/       Optional LAN service, sessions, HTTP API and still previews
qml/
  app/          Window composition and application-level dialogs
  controls/     Shared visual controls
  editor/       Canvas, layers, inspector, slide strip and notes
  libraries/    Song, Bible and media screens
  programs/     Service composition and operator panels
  templates/    Template catalog, content previews and saving
  rendering/    Shared slide/text/media rendering
  outputs/      Audience, stage and output configuration
  workspaces/   Persistent navigation, contextual toolbars and live operator view
remote/        Bundled mobile controller and speaker browser interface
third_party/   Pinned QR encoder with upstream license notices
tests/         Isolated model tests and optional visual diagnostics
docs/          Architecture, standards, testing and feature guides
scripts/       Run, format, data preparation and smoke-check helpers
assets/        Fonts, icons, Bible seed data and license notices
examples/      Importable song fixtures
```

The build produces a shared runtime target, a standalone model-test runner and an optional diagnostics library. Source locations can change without changing saved data or the public QML resource URLs. Read [architecture](docs/architecture.md) before changing state ownership or module boundaries.

## Quality checks

```sh
python3 scripts/format.py --check
cmake --build build --target church-deck_qmllint
ctest --test-dir build --output-on-failure
```

Use `python3 scripts/format.py` to apply the checked-in formatting rules. Read [coding standards](docs/coding-standards.md) and [testing](docs/testing.md) for conventions and feature-specific checks. Tests and diagnostic captures use separate application-data profiles.

For a runtime-only build, use the `release` preset. This disables tests and developer command-line diagnostics. To build and package a standalone macOS Release, run `python3 scripts/package-macos.py`. Packages, signatures, dependency audits and checksums are described in [production builds](docs/production-builds.md).

## Local data and current limits

Songs/Bibles, the media catalog and programs use separate SQLite databases in Qt's application-data directory. Presentations/autosave use `.churchdeck` JSON; managed media and thumbnails are ordinary files. Display preferences use `QSettings`. See [storage and formats](docs/storage.md).

NDI offscreen video capture and the bounded worker are implemented. **Network transmission remains unverified** because the official SDK/runtime is absent; NDI audio is not implemented. See [NDI prototype](docs/features/ndi-prototype.md).

Remote control is available on a trusted LAN over HTTP; see [pairing, permissions and limits](docs/features/remote-control.md). Physical phone/tablet and Windows validation is still needed.

Portable exports that bundle media, cloud sync, physical hot-plug/three-display validation and Windows release validation remain outstanding.

## Third-party content

The pinned Nayuki QR encoder retains its MIT notice under `third_party/qrcodegen`. Lucide icons retain their notice under `assets/LUCIDE-LICENSE`. Bundled Inter and Noto Telugu/Devanagari fonts retain their OFL notices under `assets/fonts/`. Bible sources, licenses and hashes are under `assets/scriptures/`; WEB is public domain, while the bundled Telugu/Hindi IRV translations are CC BY-SA 4.0 with Bridge Connectivity Solutions attribution.

No application license has been selected. Qt, third-party dependencies, fonts and content each retain their own distribution requirements.

User help: [Getting started and operator guides](docs/user-guide/README.md), also available offline from the book icon in the toolbar.
