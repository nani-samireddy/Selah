# Verified downloads

`VerifiedDownload` is shared by update packages and optional Luma assets. Callers
provide an HTTPS URL, destination, expected SHA-256 and byte limit. It streams
bounded chunks into `QSaveFile`, reports progress and commits only verified bytes.
Errors and cancellation discard the temporary file and preserve an existing
successful destination. Closing the cancelled save file is essential for retry.

The downloader does not select trusted releases, execute binaries or alter app
settings. Those policies belong to `UpdateManager` and `ModelInstaller`.

`UpdateTests` covers metadata/identity rejection and local checksums. The isolated
`--update-setup-smoke` command exercises successful HTTPS managed downloads,
checksum mismatch, cancellation and preserving an existing destination on retry.
