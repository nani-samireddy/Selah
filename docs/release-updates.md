# End-user updates (proposed)

Selah currently produces unsigned desktop archives through `scripts/package-desktop.py`.
The desktop workflow uploads temporary GitHub Actions artifacts; these are testing
packages, not a permanent update feed. Automatic updates are not implemented yet.

## User experience

- **Help → Check for updates** performs an explicit check and reports the installed
  version, available version, release notes, download progress or an actionable error.
- Optional background checks run at most daily and quietly tolerate offline use.
- Offer **Download update**, **Later**, and **Install and restart**. Defer installation
  until audience, stage and NDI outputs are stopped; preserve pending work first.
- Keep stable and preview channels separate. Match operating system, architecture
  and minimum supported OS version. Never propose a version older than installed.

## Platform adapters

Use a small C++ `UpdateManager` to expose state and commands to QML, with native
installation adapters rather than implementing an executable replacer in QML.

- **macOS:** Sparkle 2, bundled with Selah. Developer ID signing/notarization for
  the app, and EdDSA signatures on update archives. Configure a public verification
  key and HTTPS appcast URL in the bundle.
- **Windows:** WinSparkle with an appcast and signed update payloads. Produce a
  versioned installer rather than assuming an unpacked portable archive has an
  install location. Code-sign the installer for distribution.
- **Linux:** Initially offer the matching package download. Add an updater only
  after choosing a distribution format; package-manager installations should
  continue to use their package manager.

## Release pipeline

1. Give each release a version and increasing build number; trigger release builds
   from a version tag after tests pass.
2. Build, sign and package each supported architecture. Keep signing private keys
   in CI secrets; distribute only verification keys with the app.
3. Publish immutable packages and release notes to GitHub Releases. Publish the
   appcast only after every referenced package is available.
4. Validate signatures and perform a complete update from the previous production
   version on each supported OS before announcing the release.

The remaining setup is release signing credentials, permanent feed URLs, a Windows
installer choice and platform update-adapter integration. Ordinary local builds
must remain usable without signing secrets.

References: [Sparkle publishing](https://sparkle-project.org/documentation/publishing/),
[WinSparkle](https://winsparkle.org/).
