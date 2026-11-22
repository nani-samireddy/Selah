# Live output and routing

`OutputManager` owns the live snapshot, navigation, screen assignments and independent audience/stage controls. QML output windows render that state without taking ownership of the editor.

Protect the operator display, cancel stale deferred fullscreen transitions and hide disconnected outputs. Screen routing is asynchronous; do not assume showFullScreen alone chooses the correct macOS screen.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).

`DefaultImage` validates and copies a still image into managed profile storage. `OutputManager`
keeps the prepared preference separate from its active standby layer. `audienceElements`,
`audienceBackground` and `audienceAmbientOpacity` feed physical/NDI output; `liveElements` and the
live snapshot still describe the underlying program for stage notes and navigation. Configuration
only emits state changes; showing/resuming emits live changes without changing the snapshot ID.
Blackout remains independent and takes priority over standby. Do not implement standby by taking
a one-slide replacement snapshot: that would discard service navigation and remote continuity.
