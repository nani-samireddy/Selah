# Songs and scripture

`ContentLibrary` owns the local catalog and queries. `ScriptureImporter` owns the worker-thread SQLite transaction. `SlideFactory` shapes and splits multilingual content into editable slides.

Preserve Unicode, paired lyric rows, source attribution and repeat order. Imports must not overwrite existing translation IDs or leave partial rows/indexes.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
