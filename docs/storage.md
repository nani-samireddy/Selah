# Local storage and formats

All current storage is local. Qt chooses the platform's application-data directory using organization `ChurchDeck` and application `Church Deck`. On macOS this is `~/Library/Application Support/ChurchDeck/Church Deck/`; on Windows it normally resides under the user's roaming application-data directory. The app does not currently synchronize data to a server.

| Data | File/format | Owner |
| --- | --- | --- |
| Songs, arrangements, translations, books, verses and FTS index | `content-library.sqlite` | ContentLibrary |
| Media catalog | `media-library.sqlite` | MediaLibrary |
| Reusable content layouts and revisions | `templates.sqlite` | TemplateStore |
| Services and last-opened program | `programs.sqlite` | ProgramStore |
| Editor recovery | `editor-recovery.churchdeck`, JSON format 1 | EditorDocument |
| Imported media | `media/<SHA-256>.<extension>` | DocumentMedia / media catalog |
| Thumbnails | `thumbnails/*.jpg` | MediaLibrary |
| Audience/stage assignments | Platform-native QSettings | OutputManager |

The Bible seed is copied from bundled resources only when creating a content database. Imported translations append a new ID in a worker-owned transaction and FTS entries are added with the verse records. Songs use JSON payloads plus normalized searchable text. Programs use copied item/slide payloads and update their last-opened preference in the same transaction as saving.

Presentation and program exports use `QSaveFile` atomic replacement. The editor debounces autosave by 650 ms; program mutations debounce database writes by 450 ms. UI item-note drafts also have a short debounce before entering the program model. Unsaved song-library drafts remain session-local until Save/Add slides.

## Exchange contracts

- `.churchdeck`: JSON object with `format: 1`, `slides`, and `currentSlide`.
- Song JSON: `format: "churchdeck-song-1"` with a `song` payload.
- Bible JSON: `format: "churchdeck-bible-1"` with translation metadata and verse records.
- `.churchtemplate`: `format: "churchdeck-template-1"` with a typed slide-layout `template` payload.
- `.churchprogram`: `format: "churchdeck-program-1"` with a `program` payload.

See [content formats](features/content-libraries.md) and [programs](features/programs.md) for examples. These formats preserve Unicode and attribution metadata. Imports validate a candidate before replacing active state. Program/song imports create independent identities; existing library records/programs are not overwritten.

Exports reference managed media files by local URL; they do not package the files yet. Back up the application-data directory together with media, not only the JSON exports. Close the app before making a simple filesystem backup of SQLite databases, or use SQLite's backup facilities; copying a database while ignoring its WAL can omit recent writes.

The source-tree refactor intentionally keeps database names, JSON keys and QML resource URLs unchanged. Future schema changes must introduce migrations instead of replacing a user's database with a new seed. Undo histories are in memory and do not survive a restart.
