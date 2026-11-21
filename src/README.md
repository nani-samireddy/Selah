# C++ runtime

`app/main.cpp` is the composition root. Feature models are built into `church-deck-core`, which has no dependency on test implementations. [Architecture](../docs/architecture.md) explains state ownership and threading.

- [app](app/README.md): startup, profile selection and font registration.
- [document](document/README.md): editing, persistence, media insertion and history.
- [content](content/README.md): song/Bible queries, imports and slide generation.
- [templates](templates/README.md): reusable layout definitions, text fitting and content population.
- [fonts](fonts/README.md): complete Google Fonts catalog, explicit downloads, offline cache and license notices.
- [media](media/README.md): asset catalog and thumbnails.
- [programs](programs/README.md): curated service snapshots and exchange.
- [outputs](outputs/README.md): physical routing and live state.
- [remote](remote/README.md): optional LAN service, sessions, role checks and still previews.
- [broadcast](broadcast/README.md): offscreen capture and sender thread.

Add runtime sources explicitly to `src/CMakeLists.txt`. Tests belong under `tests/`; do not put diagnostic scenarios back into startup or feature implementation files. Existing static `selfTest()` declarations are implemented only by the test-cases target.
