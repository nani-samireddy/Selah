# Developer scripts

- `package-macos.py`: compile the runtime-only Release, bundle Qt/media dependencies with `macdeployqt`, audit Mach-O dependencies, ad-hoc sign, and produce a ZIP/DMG plus checksums in `dist/`. `--skip-build` packages a previously built Release.
- `run.sh`: configure/build and launch using Homebrew's Qt prefix when available.
- `format.py`: format C++/QML; `--check` verifies without modifying files. Uses checked-in configs and Python's standard library.
- `prepare-scriptures.py`: rebuild the offline Bible seed from the published eBible VPL archives. Downloads are cached under `build/`; source notices and hashes are retained. Do not edit verse text or silently replace a user's live database.
- `check-media.sh`: generate media fixtures and verify rendering/playback/snapshot isolation.
- `check-ndi.sh`: generate fixtures and verify offscreen capture, alpha and bounded queuing without claiming network interoperability.

Shell helpers target Unix-like development environments. Windows builds can use the CMake presets and executable diagnostic flags directly. FFmpeg is needed only by fixture-generation helpers. See [testing](../docs/testing.md).

`run.sh` uses `build.py` to serialize configure/build operations for a checkout. Agent and Terminal builds should use that wrapper rather than running concurrent Ninja processes in the same build directory. The default is four compiler jobs; set `CHURCHDECK_BUILD_JOBS` to override it. For build-only verification, use `python3 scripts/build.py`, or add `--target church-deck_qmllint`. Locks are released automatically when the helper exits.
