# Desktop updates

`UpdateManager` exposes stable GitHub release checks and explicitly requested,
checksum-verified package downloads to QML. Strict versions, supported platform
names and repository download paths are validated before offering a package.
The application version comes from CMake's project version.

`UpdateDialog` lives under Help → Check for updates. It displays status, notes,
progress, cancellation and manual platform-specific replacement instructions.
Opening the download folder is disabled while the host is presenting. No code in
this module installs an archive, launches an installer or quits the application.

See [the update flow](../../docs/release-updates.md) for package naming, publishing
requirements and the remaining signed native adapter work. No public stable
release existed during the initial implementation, so live checks validate that
empty-feed state; actual package upgrades remain a release gate.
