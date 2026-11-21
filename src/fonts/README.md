# Font library

`FontLibrary` loads the bundled complete Google Fonts metadata catalog and registers previously downloaded families before document recovery. `FontDownloads` handles explicit HTTPS downloads, pinned Git blob verification, the official native-TTF CSS fallback, cancellation, embedded font notices and cache registration.

Fonts are cached under the application data directory, not installed into the operating system. The index is written atomically after a family is ready. Existing system and bundled fonts remain available. QML requests a family, waits for `familyReady`, then applies it only if the originally selected text layer still matches.

Run `scripts/build-google-font-catalog.py` to refresh the catalog from Google's official public metadata and source tree. The source snapshot currently contains 1,961 families. Metadata ships offline; uncached font files need internet access. Actual cache files and licenses are user data and must not be committed.
