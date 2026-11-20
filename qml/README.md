# QML UI

`app/Main.qml` composes the window using required C++ model properties. Reusable components are grouped by responsibility:

| Folder | Entry points |
| --- | --- |
| `controls` | StudioButton/Icon/Panel, headings, inputs and selectors |
| `editor` | EditorWorkspace, EditorCanvas, InspectorPanel, LayersPanel, SlideStrip, SpeakerNotes, EditorDock |
| `libraries` | SongLibraryPanel, BibleLibraryPanel, MediaLibraryPanel |
| `programs` | ProgramPanel composes ProgramOrder, ProgramItemEditor and ProgramLiveControls |
| `templates` | TemplateLibraryPanel, TemplateSelector and SaveTemplateDialog |
| `rendering` | SlideRenderer, SlideLayer, MediaLayer, BroadcastScene |
| `remote` | RemotePanel for pairing codes, network selection and device revocation |
| `outputs` | PresentationOutput, StageView and output configuration |

Model mutations stay in C++; components own only focus, input drafts, selection previews and transient gesture state. Editor children receive an explicit `host` interface; program children receive `programView`. Prefer required properties/signals to private cross-file IDs.

CMake discovers QML files and keeps stable aliases `qml/<Type>.qml` inside the ChurchDeck module. Filenames must be unique. These aliases keep relative asset paths and the offscreen scene URL working despite physical source folders. Do not change them casually.

Run `python3 scripts/format.py --check` and `cmake --build build --target church-deck_qmllint`. After component extraction, run the relevant rendering scenario and inspect screenshots, including the 1120×760 compact window. See [UI design](../docs/ui-design.md) and [testing](../docs/testing.md).
