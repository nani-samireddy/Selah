# Live output and routing

`OutputManager` owns the live snapshot, navigation, screen assignments and independent audience/stage controls. QML output windows render that state without taking ownership of the editor.

Protect the operator display, cancel stale deferred fullscreen transitions and hide disconnected outputs. Screen routing is asynchronous; do not assume showFullScreen alone chooses the correct macOS screen.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
