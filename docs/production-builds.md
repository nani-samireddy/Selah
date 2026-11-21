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
