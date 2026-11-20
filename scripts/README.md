# Developer scripts

- `run.sh`: configure/build and launch using Homebrew's Qt prefix when available.
- `format.py`: format C++/QML; `--check` verifies without modifying files. Uses checked-in configs and Python's standard library.
- `prepare-scriptures.py`: rebuild the offline Bible seed from the published eBible VPL archives. Downloads are cached under `build/`; source notices and hashes are retained. Do not edit verse text or silently replace a user's live database.
- `check-media.sh`: generate media fixtures and verify rendering/playback/snapshot isolation.
- `check-ndi.sh`: generate fixtures and verify offscreen capture, alpha and bounded queuing without claiming network interoperability.

Shell helpers target Unix-like development environments. Windows builds can use the CMake presets and executable diagnostic flags directly. FFmpeg is needed only by fixture-generation helpers. See [testing](../docs/testing.md).
