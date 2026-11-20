# Architecture

Church Deck is a native Qt Quick application. QML owns views and transient interaction state. C++ models own persisted data, validation and live state. An optional Qt Network HTTP server supplies a bundled browser remote on a selected private IPv4 interface; the editor itself remains native. There is no cloud dependency.

## Build boundaries

- `church-deck-core`: application setup helpers and runtime models. It has no test implementation dependency.
- `church-deck`: the short composition root in `src/app/main.cpp` plus QML/resources.
- `church-deck-test-cases`: model-test implementations, shared by the standalone runner and optional diagnostic command.
- `church-deck-tests`: standalone model runner with its own font/Bible resource fixtures.
- `church-deck-diagnostics`: optional capture scenarios under `tests/smoke/`. Disable with `CHURCHDECK_ENABLE_DIAGNOSTICS=OFF`.

Application startup constructs the controllers before the QML engine, marks them as C++ owned, then injects them through required root properties. The engine therefore cannot delete stack-owned controllers, and UI dependencies are visible in `Main.qml`. The engine is destroyed before the controllers it references. NDI and the remote service are stopped when the application quits.

## Three independent states

1. `EditorDocument` is the editable presentation draft. It owns element validation, undo/redo and debounced autosave.
2. `ProgramStore` owns curated service items and their copied slide/content snapshots. Editing a library song or editor document does not alter an item unless explicitly added/updated.
3. `OutputManager` owns a copied live snapshot. Only an explicit take-live operation replaces it; navigation moves within that copy. Program item IDs embedded in live slides keep operator highlighting stable even when draft items move or are removed.

Preserve these boundaries when adding features. Do not bind audience output directly to editor/program draft lists.

## Runtime modules

| Module | Responsibility |
| --- | --- |
| `document` | Draft editing; `DocumentPersistence` validates/serializes; `DocumentMedia` manages asynchronous insertion; `DocumentElements` constructs stable element identities. |
| `content` | SQLite song/Bible catalog; `ScriptureImporter` owns worker-thread import transactions; `SlideFactory` shapes and splits content. |
| `media` | Managed asset catalog, discovery, missing-file detection and thumbnail scheduling. |
| `templates` | Versioned layout catalog, validation, shaped text measurement and bound content composition. |
| `programs` | Curated services and item edits; `ProgramPersistence` handles database/exchange operations; `ProgramSlides` creates independent renderer identities. |
| `outputs` | Screen detection, routing, operator protection, live navigation and private stage state. |
| `remote` | Opt-in LAN pairing, ephemeral sessions, role-enforced commands and cached still previews of the immutable live snapshot. |
| `broadcast` | GUI-thread QRhi capture and a bounded sender thread; optional NDI SDK/runtime integration. |

## Threads and resources

All QML-facing controller objects live on the GUI thread. File copying and Bible import use QtConcurrent. A Bible import creates its own named SQLite connection in the worker and closes/removes it there; Qt SQL connections must not cross threads. Media thumbnail decoding is scheduled through Qt multimedia on the owning thread.

NDI rendering stays on the GUI thread. Readback frames are handed to `NdiWorker`, which keeps one in-flight frame and one replaceable pending frame. It drops superseded pending work instead of accumulating latency. Stop the worker before destroying its capture resources.

`SlideRenderer`, `SlideLayer` and `MediaLayer` are shared by editor previews and outputs. Reconciliation uses element IDs to preserve media delegates through unrelated edits. Duplicated/imported snapshots receive fresh IDs. Thumbnail views disable video decoding.

## QML composition

`qml/app/Main.qml` wires required model properties, application dialogs, shortcuts and top-level windows. `EditorWorkspace` handles panel placement; canvas, inspector, layers, notes, strip and dock are separate files. Programs separates order, selected-item editing and live controls. Shared controls belong in `qml/controls`, while the renderer must remain usable without operator widgets.

Editor components receive an explicit `host` command/state interface from the application window. Program subviews receive `programView`. These are view composition contracts, not separate owners of persisted state. Prefer dedicated properties/signals for new independent components; avoid reaching across files through private IDs or adding hidden context globals.

QML source files are grouped by feature, but CMake retains resource aliases `qml/<Type>.qml`. This preserves `loadFromModule`, relative asset URLs and the broadcast scene URL. QML type filenames must be unique across the module; configuration rejects duplicate names.

## Current limits

Persistent records still use QVariant maps and validated JSON payloads; typed value models and schema migrations will be needed as formats evolve. The unit cases retain member `selfTest()` hooks to exercise existing private validation paths, but all test bodies are outside runtime sources and are omitted from runtime-only builds. Whole-program data duplication, portable media packaging and encrypted remote transport remain separate changes. Template catalog updates are explicit; applied definitions and source content stay copied in prepared programs.
