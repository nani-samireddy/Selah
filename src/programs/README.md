# Service programs

`ProgramStore` owns curated item mutations and live compilation. `ProgramPersistence` contains SQLite/exchange operations. `ProgramSlides` creates independent element identities when copying snapshots.

Editing, reordering or removing draft items must not alter the current live snapshot. Update-from-editor and take-live are explicit operations; keep item notes private to the stage view.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
