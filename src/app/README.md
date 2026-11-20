# Startup

`main.cpp` creates controllers, sets explicit C++ ownership, injects required root properties and starts the QML engine. `ApplicationSetup` selects profiles before any model/settings object is constructed and registers bundled Indic fonts.

Keep feature behavior and diagnostic scenario bodies out of startup. The diagnostics entry point is conditional; runtime-only builds do not link test bodies.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
