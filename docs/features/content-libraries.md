# Songs and Bible libraries

Open **Libraries → Songs** or **Libraries → Bible** in the editor. Both libraries are stored locally in `content-library.sqlite` in the app's data folder. Songs are separate from presentation files. Adding content creates editable slide snapshots; changing a library item later does not silently change a presentation or live output.

## Songs

Create a song, enter its title, language names, attribution and optional tags. Each section has a stable key (`v1`, `c1`, etc.), a display name, primary lyrics and optional translated lyrics. Use matching rows for translated lines. A blank row in both languages forces a slide break. Sections may be named freely, including bridges and outros.

Type section keys into the arrangement in presentation order, for example `v1 c1 v2 c1`. Keys may repeat. **+ Arrangement** creates a named alternative using the same lyrics. Save stores all alternatives and the selected arrangement. **Add slides** saves and appends the selected arrangement as one undoable operation. Line count is a maximum: shaping, available space and long-line wrapping may produce fewer lines per page. Generated slides include credits and remain editable.

Unsaved drafts survive switching between songs and closing/reopening the library within the current app session. Save before quitting the app. The library starts with no copyrighted song lyrics. Import/export currently supports Church Deck JSON, not OpenLyrics, SongSelect, ProPresenter or FreeShow formats.

Song exchange example:

```json
{
  "format": "churchdeck-song-1",
  "song": {
    "title": "An original song",
    "primaryLanguage": "English",
    "secondaryLanguage": "తెలుగు",
    "credits": "Author / copyright / permission",
    "tags": "Gathering",
    "sections": [
      {"key":"v1","label":"Verse 1","primary":"We gather in hope","secondary":"ఆశతో కూడుదము"}
    ],
    "arrangement": ["v1"],
    "arrangements": [{"name":"Default","order":["v1"]}],
    "activeArrangement": 0
  }
}
```

Imports get a new identity, so importing cannot overwrite an existing song. Search includes title, lyrics in both languages, credits and tags.

## Bible

Three complete 66-book translations are bundled and available offline:

| Translation | Language | Terms |
| --- | --- | --- |
| World English Bible (WEB) | English | Public domain |
| Indian Revised Version (IRV) | Telugu | CC BY-SA 4.0; © 2017, 2019 Bridge Connectivity Solutions |
| Indian Revised Version (IRV) | Hindi | CC BY-SA 4.0; © 2017, 2018, 2019 Bridge Connectivity Solutions |

Sources: [WEB](https://ebible.org/bible/details.php?id=engwebp), [Telugu](https://ebible.org/bible/details.php?id=tel2017), [Hindi](https://ebible.org/bible/details.php?id=hin2017). The original notices and source hashes are retained under `assets/scriptures/`. Verse text is copied unchanged from the published eBible VPL archives. Internal book identifiers are normalized to USFM codes. English book labels are used for navigation across the bundled versions.

Browse by version, book, chapter and verse range, or search a reference such as `John 3:16-18` / `JHN 3:16`, or an exact phrase in the selected language. Search shows up to 100 results. Select individual results or add the displayed passage. Each verse starts on a new slide; long verses split into numbered continuations. References and version abbreviations appear on slides; source copyright and licence are stored in the slide metadata. Each add operation can be undone as a batch.

The library accepts additional translation packages through **Import translation**. It validates and indexes the package in a worker thread and rolls back the entire import on invalid or duplicate verse records. Existing translation IDs cannot be replaced. A package may contain a partial translation.

```json
{
  "format":"churchdeck-bible-1",
  "translation":{
    "id":"my_translation",
    "title":"My translation",
    "abbreviation":"MT",
    "language":"English",
    "copyright":"Rights holder or public domain notice",
    "license":"Distribution terms",
    "source":"Source URL or attribution"
  },
  "books":[{"code":"JHN","name":"John"}],
  "verses":[{"book":"JHN","chapter":3,"verse":16,"text":"Your permitted translation text"}]
}
```

Book IDs use the standard 66-book USFM codes; localized book names are optional. Supply translations you have permission to use. This package format does not import commercial Bible files or unlock licensed modules. Bundled Noto Sans Telugu and Noto Sans Devanagari fonts support consistent shaping on Windows and macOS; their OFL notices are under `assets/fonts/`. Other scripts use installed system font fallbacks.

## Development checks

```sh
ctest --test-dir build --output-on-failure
cmake --build build --target church-deck_qmllint
build/church-deck.app/Contents/MacOS/church-deck --fresh --content-smoke "$PWD/captures/content"
```

The smoke run uses a separate verification profile and original sample lyrics, opens both native panels, searches Telugu, generates slides, verifies atomic undo/redo and renders a Telugu audience output. It does not edit the normal user's library or recovery file. Rebuild the checked-in seed using `python3 scripts/prepare-scriptures.py` (network required only for initial source downloads). The application itself does not require a network connection for library use.

Programs are now implemented; see [PROGRAMS.md](programs.md). Reusable template population is also implemented; see [templates](templates.md). Song/Bible synchronization with external providers, transliteration, bilingual Bible slides and app-wide localization remain separate work.
