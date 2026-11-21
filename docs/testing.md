# Testing and diagnostics

Configure the normal development build with `BUILD_TESTING=ON` and `CHURCHDECK_ENABLE_DIAGNOSTICS=ON`. Defaults are enabled. The `release` preset disables both.

```sh
cmake --build build
python3 scripts/format.py --check
cmake --build build --target church-deck_qmllint
ctest --test-dir build --output-on-failure
```

`tests/church-deck-tests` is a standalone model runner. It covers draft editing/round-trips, content imports/search, shaping/splitting, media catalog state, service snapshots/persistence live routing state and loopback HTTP remote sessions/commands. Fixtures use temporary databases and Qt's test data profile. Test definitions are stored in `tests/unit`, separate from runtime implementation files. New cases should check behavior and data safety rather than mirror an implementation.

## Visual smoke checks

The following macOS executable paths assume the development build. On Windows, use `build/Selah.exe` instead. Create the capture folder before using standalone screenshot mode.

```sh
mkdir -p captures
build/Selah.app/Contents/MacOS/Selah --fresh --screenshot "$PWD/captures/editor.png"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --template-smoke "$PWD/captures/templates"
build/Selah.app/Contents/MacOS/Selah --fresh --workspace-smoke "$PWD/captures/workspaces"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --workspace-smoke "$PWD/captures/workspaces-compact"
build/Selah.app/Contents/MacOS/Selah --fresh --content-smoke "$PWD/captures/content"
build/Selah.app/Contents/MacOS/Selah --fresh --program-smoke "$PWD/captures/program"
build/Selah.app/Contents/MacOS/Selah --fresh --compact --program-smoke "$PWD/captures/program-compact"
build/Selah.app/Contents/MacOS/Selah --fresh --routing-smoke "$PWD/captures/routing"
./scripts/check-media.sh
./scripts/check-ndi.sh
build/Selah.app/Contents/MacOS/Selah --fresh --remote-demo "$PWD/captures/remote"
```

The remote demo stays running for browser integration checks; close it when finished. It writes test-only pairing data and binds loopback, never a public interface. See the [remote guide](features/remote-control.md).

Media/NDI helper scripts generate fixtures with FFmpeg; FFmpeg is not an application dependency. `--physical-smoke <folder>` routes real output to a connected non-operator display and should only be run when those displays are available for testing. Physical unplug/reconnect and three-display operation are still unverified.

Profiles are selected before constructing models or reading settings. `--fresh` skips recovery within that run's selected profile; it is not a request to erase normal data. Screenshot mode uses a capture profile; other diagnostics use a verification profile. Run visual scenarios sequentially so separate windows and test-profile writes cannot interfere with each other.

Inspect the process exit status and log, then view the generated images/reports. Check text shaping, clipping, live/draft isolation, current/next contents and stage-only notes. SDK-free NDI checks verify capture/alpha/queuing only; they do not prove network transmission or audio interoperability.

## Runtime-only build

```sh
cmake --preset release -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build --preset release
```

This build excludes model-test bodies and smoke commands. Verify this configuration after changing build boundaries. For self-contained macOS packages, use `python3 scripts/package-macos.py`. The helper audits dependencies, verifies ad-hoc signatures, checks DMG integrity and records checksums. Developer ID notarization and Windows deployment remain separate release work.
