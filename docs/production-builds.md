# Production builds

The Release preset uses optimized compilation and excludes test bodies and developer smoke/capture commands. Build the native runtime with the release preset or use the macOS packaging helper:

```sh
python3 scripts/package-macos.py
```

The helper requires Python 3.11+ and uses the existing cross-process build lock. Two compiler jobs are the default; `--jobs` changes that. `--skip-build` packages an already compiled Release, after checking that tests and developer diagnostics are disabled. It stages a copy so deployment never modifies the build directory's app bundle.

## macOS artifacts

`dist/Selah.app` contains the runtime, Qt frameworks, imported QML modules, plugins, media dependencies and bundled notices. `macdeployqt` deploys dependencies; the helper rejects unresolved or external development-library links, derives the minimum macOS version from the actual Mach-O binaries, writes version/platform metadata, and verifies the final ad-hoc signature.

The ZIP and DMG names include the project version and executable architecture. The DMG contains Selah and an Applications shortcut. Each archive has a SHA-256 sidecar; `release-manifest.json` records checksums, sizes, architecture, OS floor, Qt version, diagnostics flags, signing mode and NDI compilation status. `README-release.txt` summarizes the package's limits.

This is an ad-hoc-signed local release candidate. Public Apple distribution still needs an appropriate Developer ID certificate and notarization. The helper does not upload the app or use Apple account credentials.

## Dependency limits

The current Homebrew FFmpeg installation targets macOS 27, so packages made on this machine require macOS 27 or newer and Apple Silicon. Older macOS support requires rebuilding the toolchain and media libraries for that target; changing only the app's deployment-target number cannot provide compatibility.

Network NDI sending is compiled only when the official NDI SDK headers are available at configuration time. Check `ndiSenderCompiled` in the manifest. Packaging cannot add sender support to a build compiled without the SDK. No NDI license agreement is accepted or SDK downloaded by this helper.

Saved data and preferences retain the existing storage namespace across development and release builds. Windows packaging requires a Windows Qt toolchain and is not produced by the macOS helper.

## Windows and Linux packages

Use Qt 6.11 or newer; the native desktop workflow uses Qt 6.12.1 with Multimedia,
ShaderTools, QuickTimeline and Quick3D. Windows uses the MSVC 2022 x64 toolchain;
run the helper from its developer shell. Linux needs Ninja, patchelf, XCB/XKB,
OpenGL/EGL, fontconfig, PulseAudio/ALSA, and VA-API runtime libraries.

```sh
python scripts/package-desktop.py --jobs 2
```

The helper configures the Release preset, deploys Qt/QML dependencies with Qt's
native deployment API, includes notices, and produces a ZIP on Windows or tar.gz
on Linux under `dist/<platform>-<architecture>/`. Extract the entire package and
launch `Selah.cmd` on Windows or `./Selah` on Linux. Keep its library and plugin
folders together. Windows packages include the app-local MSVC runtime.

`.github/workflows/build-desktop.yml` builds Windows x64 and Linux x64 on native
GitHub runners. Pushes to `build/**` trigger it; the workflow also supports manual
runs when available on the repository's default branch. Artifacts are retained for
14 days. This workflow does not create a public release, merge a branch, or sign
Windows executables. Its pinned aqt source revision supports the split Windows Qt
repository introduced with Qt 6.11; the older PyPI aqt release cannot install that SDK.

Each package includes a manifest and SHA-256 sidecar. The helper verifies headless
startup with development plugin paths removed. Linux also audits ELF dependencies
and rejects missing libraries or Qt libraries resolved outside the bundle. The
manifest records its distro/glibc floor: a package built on Ubuntu 24.04 targets
Ubuntu 24.04 or compatible newer distributions, not every Linux installation.
Headless startup is not a physical display, hardware decoding, or NDI receiver test.

`packaging/Dockerfile.linux` describes a native Linux SDK container for x64/ARM64
builders. It requires sufficient local disk space and a running Docker daemon;
Windows executables still require a Windows runner.

For a website installer, add `--windows-installer --windows-publisher "Your legal name"`
on Windows with Inno Setup 6 installed. It creates an EXE installer alongside the ZIP.
See [Windows signing and installer setup](windows-release.md). Unsigned CI installers
are testing builds; installation and upgrade acceptance remain release gates.
