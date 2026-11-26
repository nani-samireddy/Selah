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

## Spontaneous additions

`QuickAddController` stages previews using the existing content factories, saved
libraries and installed Bibles. It never receives an editor or ProgramStore.
Search, draft editing and previews cannot replace the live snapshot. Quick Song
and text drafts, plus translation choices, are bounded and stored in profile
preferences; saving to a library requires a separate operator action.

`OutputManager::addQuickSlides` inserts a distinct, labeled group into the loaded
session. Queue insertion preserves the current index, playback, blackout and
clear-text state, and does not prepare a transition. Present now navigates to the
new group and keeps a return index. Resume restores that position; for queued
content reached by normal navigation it goes to the next original item. Loading
a program replaces this session queue. Additions are not written to saved
programs or restored as active output on relaunch.

Unit checks cover insertion boundaries/order, resume, exact scripture, draft
splitting and explicit library saving. `--quick-add-smoke <folder>` checks the QML
preview/present/resume/queue flow and captures the compact editor strip.
