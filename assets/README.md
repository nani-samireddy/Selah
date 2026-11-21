# Bundled assets

- `ambient.svg`: project-authored vector background.
- `icons/`: Lucide icon set; retain `LUCIDE-LICENSE`.
- `fonts/`: Inter for the library interface, Noto Sans Telugu/Devanagari and their SIL Open Font License notices.
- `scriptures/`: offline SQLite seed, source manifest/hashes and the original translation notices.

Fonts are registered during application setup. The Bible seed is copied into application data at first use. The data-preparation script maps source book codes to the internal USFM identifiers without editing verse text. Preserve attribution and notices when updating assets. Runtime-only builds still need these resources.
