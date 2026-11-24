# Presentation fonts

In **Design**, select a text layer and open **Font family** in the inspector. The picker contains the full bundled Google Fonts catalog, with search, script filters, offline availability and system fonts. The current catalog snapshot lists 1,961 families.

Choose a family and use **Download & use**. The app downloads native font files from the official Google Fonts source, registers them in this application and caches them locally. Fonts already available use **Use font** immediately. Downloaded families register before recovered presentations load on subsequent launches, so they work offline. Cancellation and network errors leave the text layer unchanged. If selection changes during a download, the font is cached without being applied to another layer.

Selecting a catalog family automatically fetches one representative face for a private
preview after a short debounce. The selected text is rendered from those font bytes;
the family is not registered application-wide or marked available offline. Preview
requests send the family name, never the selected lyrics or other preview text.
Recent previews use a bounded session-only memory cache; switching families or
closing the picker cancels the outstanding request. A failed request offers Retry
preview. The preview identifies unsupported characters instead of showing a different
font as if it were the selected family.

Installed/downloaded families use the normal text preview with script fallbacks. Telugu and Devanagari retain the bundled Noto fallbacks. System font availability varies by computer. Variable font files preserve their font data, while the current editor controls family and size; dedicated weight/axis controls remain future work.

Downloaded licenses and embedded copyright/license notices are available through **License & attribution**. Fonts stay in the application cache and are not installed system-wide. Exported `.churchdeck`/program files retain family names and do not yet package font binaries; download the same fonts on another computer before presenting. Prepare required fonts before a service, particularly when opening content that previously used unavailable fonts.

## Storage and verification

Cache location: Qt application data directory, `fonts/google/`. Each family has a stable hashed directory, native font files and a readable `NOTICE.txt`; `index.json` records completed cache entries. Source repository files are checked against the pinned Git blob IDs. If the repository host cannot be reached, the downloader can use Google's official CSS/native-TTF endpoint. Font files have bounded sizes and transfers are cancellable.

The normal model checks cover full-catalog/script search, trusted TTF URL parsing, notice extraction, cache reload and system font queries. An explicit integration check downloads two representative families and reloads them from cache without repeating downloads:

```sh
build/tests/church-deck-tests --fonts-network "$PWD/captures/fonts"
```

Use `python3 scripts/build-google-font-catalog.py` to update the bundled metadata snapshot as Google's catalog grows. Windows and unusual/color font rendering still need platform-specific verification; fonts unsupported by the native renderer report an error instead of being silently selected.

`--fresh --font-preview-smoke captures/font-preview` exercises automatic online
preview loading and the native preview canvas with Telugu/English text, checks
that the family remains uninstalled and the document/live snapshot stay unchanged,
and verifies cleanup on close. `--fonts-network` also covers private preview loading
before the full install for both repository and Google CSS sources.
