# Songs and scripture

`ContentLibrary` owns the local catalog and queries. `ScriptureImporter` owns the worker-thread SQLite transaction. `SlideFactory` shapes and splits multilingual content into editable slides.

Preserve Unicode, paired lyric rows, source attribution and repeat order. Imports must not overwrite existing translation IDs or leave partial rows/indexes.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).

`BibleXmlReader` normalizes Beblia and Zefania XML to the shared JSON-package validation/persistence
pipeline. `BibleRepository` owns the catalog, repository URL normalization and bounded asynchronous
HTTPS downloads. `ContentLibrary` coordinates those stages and emits `bibleImported` only after the
transaction commits, so the UI can select the new edition without guessing its identity.

Refresh the repository metadata using `python3 scripts/build-bible-catalog.py`; it uses the Git-tree
API because GitHub's directory contents endpoint truncates this repository at 1,000 entries. The
catalog records its source commit. Verse text is fetched only when an operator requests an import.
