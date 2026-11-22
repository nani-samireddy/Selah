# QML UI

`app/Main.qml` composes the window using required C++ model properties. Reusable components are grouped by responsibility:

| Folder | Entry points |
| --- | --- |
| `workspaces` | WorkspaceNavigation, LibraryWorkspaceBar, DesignWorkspaceBar, PresentWorkspace and LiveSlideCard |
| `controls` | StudioButton/Icon/Panel plus the shared LibraryStyle, fields, selects, tabs, textareas and numeric controls |
| `editor` | EditorWorkspace, EditorCanvas, InspectorPanel, LayersPanel, SlideStrip, SpeakerNotes, EditorDock |
| `fonts` | FontPicker for Google/system families, preview and offline downloads |
| `libraries` | SongLibraryPanel, BibleLibraryPanel, MediaLibraryPanel |
| `programs` | ProgramPanel composes ProgramOrder, ProgramItemEditor and ProgramLiveControls |
| `templates` | TemplateLibraryPanel, TemplateSelector and SaveTemplateDialog |
| `rendering` | SlideRenderer, SlideLayer, MediaLayer, BroadcastScene |
| `remote` | RemotePanel for pairing codes, network selection and device revocation |
| `outputs` | PresentationOutput, StageView and output configuration |

Model mutations stay in C++; components own only focus, input drafts, selection previews and transient gesture state. Editor children receive an explicit `host` interface; program children receive `programView`. Prefer required properties/signals to private cross-file IDs.

CMake discovers QML files and keeps stable aliases `qml/<Type>.qml` inside the ChurchDeck module. Filenames must be unique. These aliases keep relative asset paths and the offscreen scene URL working despite physical source folders. Do not change them casually.

Run `python3 scripts/format.py --check` and `cmake --build build --target church-deck_qmllint`. After component extraction, run the relevant rendering scenario and inspect screenshots, including the 1120×760 compact window. See [UI design](../docs/ui-design.md) and [testing](../docs/testing.md).

The `TextInspector` and stable `RichTextEditor` share the `ChurchDeck.Formatting` module. `SongSectionsEditor` keeps keyed editable section cards alive while the song model changes. `SongOrderEditor` edits named sequence entries independently of the lyric content. `ScriptureNumberGrid` is shared by chapter and verse navigation. Appearance preferences and neutral colors live in `LibraryStyle`.

Layers are listed front to back. Dragging a row previews an insertion line, auto-scrolls the list, and commits a single document move on release. Escape and outside drops cancel. The compact header contains forward/backward, duplicate, delete, and add icons. `LayerOrderDrag` owns only the transient gesture; `EditorDocument.moveElementTo` owns persistent order and preserves selection by UID.

`menus/ApplicationMenuBar.qml` owns desktop application menus and their shared
shortcuts. Route commands through the Main window's workflow functions rather
than duplicating persistence or presentation behavior inside menu handlers.
