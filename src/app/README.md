# Startup

`main.cpp` creates controllers, sets explicit C++ ownership, injects required root properties and starts the QML engine. `ApplicationSetup` selects profiles before any model/settings object is constructed and registers bundled Indic fonts.

Keep feature behavior and diagnostic scenario bodies out of startup. The diagnostics entry point is conditional; runtime-only builds do not link test bodies.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).

The operator window uses Qt 6.9 expanded client areas to share the native caption area with the workspace toolbar. Native window buttons and resize borders remain owned by the OS. `WindowChrome.mm` hides only the visible macOS caption; the title remains available to accessibility and window switching. Drag the empty toolbar area to move the window and double-click it to maximize or restore. Windows placement reserves space for its native caption buttons; it still needs verification on Windows hardware.
