# End-user updates

Selah v1 implements explicit checks and verified downloads from the stable GitHub
release in `nani-samireddy/Selah`. Open **Help → Check for updates**. The dialog
shows the installed version and platform, release notes, download progress,
cancellation and connection errors. No checks or downloads run automatically.

No stable release currently exists in that repository (checked 25 November 2026).
The app reports that condition explicitly. GitHub Actions artifacts are temporary
build outputs and are not an update feed.

## Current installation flow

1. Select **Download update** when a newer matching package is available.
2. Selah streams it into Downloads and verifies its SHA-256 against GitHub's asset
   digest before exposing the completed file. Cancelled, truncated and mismatched
   files do not replace a previous download. Reopening the folder verifies it again.
3. Stop audience, stage and NDI outputs. The dialog enables **Open download folder**.
4. On macOS, open the DMG or ZIP, quit Selah and replace the app in Applications.
   On Windows, extract the ZIP into a new folder, quit the old app and start the
   new folder's `Selah.cmd`. Keep the previous app copy until the new one works.
5. Reopen Selah. The existing ChurchDeck storage/settings namespace is preserved.
   Quit through the app normally so drafts and the current program are saved.

The app does not replace itself, run an unsigned installer, quit automatically or
restart itself. Current packages are unsigned archives; their checksum is an
integrity check, not code-signing identity verification. Review the release notes
for OS requirements before replacing an installation. Older versions and preview
releases are not offered through the stable updater.

## Publishing compatible assets

The release tag must be a strict version such as `v0.2.0`. Publish archives from the
release commit with their exact packaging names:

- `Selah-0.2.0-macOS-arm64.dmg` (or `.zip`)
- `Selah-0.2.0-macOS-x86_64.dmg` (or `.zip`, only if independently built/tested)
- `Selah-0.2.0-Windows-x64.zip`
- `Selah-0.2.0-Linux-x64.tar.gz`

GitHub's release asset API must report `state: uploaded`, a positive size and a
`sha256:…` digest. Without a matching verified asset, Selah displays the new version
and notes but disables Download. Source-code archives are never selected. Include
OS requirements and known issues in the release notes. Publish permanent immutable
assets and checksums; do not silently replace a version's binaries.

## Future automatic installation

An automatic **Install and restart** flow still requires native adapters and signed
release packages. Use Sparkle 2 on macOS (Developer ID/notarization plus EdDSA-signed
update archives) and WinSparkle with a signed Windows installer. These are not
bundled yet. Installation must wait for outputs to stop and work to be saved.
Validate an actual upgrade on clean machines before advertising automatic updates.

References: [GitHub releases](https://docs.github.com/en/rest/releases/releases),
[release asset digest](https://docs.github.com/en/rest/releases/assets),
[Sparkle publishing](https://sparkle-project.org/documentation/publishing/),
[WinSparkle](https://winsparkle.org/).
