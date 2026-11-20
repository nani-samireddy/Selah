# Editable presentations

`EditorDocument` owns editing, selection and undo/redo. `DocumentPersistence` validates candidates and handles snapshots/atomic files. `DocumentMedia` handles managed file imports and asynchronous insertion. `DocumentElements` constructs text elements with stable IDs.

Never point audience output at this mutable draft. An asynchronous import must retain its original target slide and detect a project replacement before inserting.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
