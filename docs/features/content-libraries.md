# Songs and Bible libraries

Open **Libraries → Songs** or **Libraries → Bible** in the editor. Both libraries are stored locally in `content-library.sqlite` in the app's data folder. Songs are separate from presentation files. Adding content creates editable slide snapshots; changing a library item later does not silently change a presentation or live output.

## Songs

Create a song, enter its title, language names, attribution and optional tags. Each section has a stable key (`v1`, `c1`, etc.), a display name, primary lyrics and optional translated lyrics. Use matching rows for translated lines. A blank row in both languages forces a slide break. Sections may be named freely, including bridges and outros.

Type section keys into the arrangement in presentation order, for example `v1 c1 v2 c1`. Keys may repeat. **+ Arrangement** creates a named alternative using the same lyrics. Save stores all alternatives and the selected arrangement. **Add slides** saves and appends the selected arrangement as one undoable operation. Line count is a maximum: shaping, available space and long-line wrapping may produce fewer lines per page. Generated slides include credits and remain editable.

Unsaved drafts survive switching between songs and closing/reopening the library within the current app session. Save before quitting the app. The library starts with no copyrighted song lyrics. Import/export currently supports Selah JSON, not OpenLyrics, SongSelect, ProPresenter or FreeShow formats.

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
build/Selah.app/Contents/MacOS/Selah --fresh --content-smoke "$PWD/captures/content"
```

The smoke run uses a separate verification profile and original sample lyrics, opens both native panels, searches Telugu, generates slides, verifies atomic undo/redo and renders a Telugu audience output. It does not edit the normal user's library or recovery file. Rebuild the checked-in seed using `python3 scripts/prepare-scriptures.py` (network required only for initial source downloads). The application itself does not require a network connection for library use.

Programs are now implemented; see [PROGRAMS.md](programs.md). Reusable template population is also implemented; see [templates](templates.md). Song/Bible synchronization with external providers, transliteration, bilingual Bible slides and app-wide localization remain separate work.

Song language fields use dropdowns covering English and the 22 scheduled Indian languages. Imported language names remain selectable. The optional second language has an explicit single-language choice. The most recently chosen pair and up to six recent languages are stored in the local `SongLanguages` settings category. New songs inherit these choices; opening an existing song keeps its saved languages.

## Song overview

All sections are shown as cards in a continuous overview. Single-language songs use two columns when space permits; bilingual sections pair the primary and translated lyrics within each card. Editors expand with their content instead of hiding long lyrics behind an inner scroll area. Section names remain visible; the Details toggle exposes names, credits, and tags for editing.

Song order uses named section chips. Add a section to repeat it; a chip's action menu edits its lyrics, moves it earlier/later, or removes that occurrence from the order. Removing an occurrence leaves its lyrics available in the section overview. Alternate named orders remain available.

## Bible navigation

The sidebar contains translation/book dropdowns, a chapter grid, and a verse-number grid. Choosing a book opens chapter one. The reading area shows the complete chapter, and verse-grid navigation scrolls to and highlights that verse. Grid clicks select that verse for addition; the main verse cards support a multiple-verse selection. Search remains available and does not replace the chapter/verse navigation. Large chapter and verse grids scroll independently to keep both available in the sidebar.

### Song tags

Tags are visible below the song languages as removable chips. Type a tag and press Enter or click +; commas, semicolons, and line breaks separate multiple tags. Spaces remain part of a label, so “Holy Spirit” is one tag. Existing library tags are suggested as you type. Save song persists edits; export/import includes the tags.

The sidebar tag filter matches a complete tag, without case sensitivity, and combines with the text search. Text search also matches tags. Song rows show their saved tags. Multilingual labels are Unicode-normalized and duplicate labels are removed. Clearing the last use of a tag removes it from suggestions and filters. Older string metadata and imported tag arrays remain compatible; storage uses a comma-separated string inside the existing song JSON, with no database migration.

### Custom slides

Libraries → Custom slides catalogs named custom presentations separately from songs, scripture, media, and templates. New presentation creates a blank slide and opens it in Design. Edit opens one saved presentation in the same editor. The editor no longer offers a Design/Template mode switch: its context comes from the item opened in Libraries.

Save presentation updates that library entry; Save a copy creates another entry. Uncataloged drafts and program items can be saved through Save to library. The selected custom presentation's ID/name are remembered with local preferences alongside the existing recovery document. Templates retain their own Save template flow.

Add to program copies the saved slides into the currently open program. Editing the library source later does not modify that prepared program or live output. Custom presentations persist in the local `presentations.sqlite` database using validated slide JSON; editor recovery and exported `.churchdeck` files remain separate.

### Add to a program

Songs, Bible passages, media, and custom presentations use a shared split button labeled **Add to program: {program title}**. It defaults to the currently open program. The primary action adds to the displayed destination; the arrow lists the other saved programs, with dates to distinguish matching titles. Choosing a menu entry immediately adds the item there and updates the button's destination.

Adding to another program keeps the library open and leaves the current program's selection, pending edits, undo history, and live output intact. The destination is saved atomically without changing the last-opened program preference. An add to the current program keeps the usual undo behavior. Switching the open program resets the split button's destination to that program.

### Quick Song

Quick Song opens a modal with only Name, Lyrics, and Tags. A blank line (including whitespace-only lines) starts a new section; single line breaks remain inside the section. Consecutive gaps do not create empty sections. Sections are named Section 1, Section 2, and so on, with a default song order that includes each section once. Windows, Unix, and legacy Mac line endings are accepted.

Saving creates a normal library song and opens it in the full song editor for later refinement. Tags entered without pressing + are included on Save. Quick Song uses the most recently chosen primary language and starts as a single-language song. Cancel leaves the existing song and library unchanged. It does not add anything to a program or alter live output.

### Import XML Bibles from the repository

Open **Libraries → Bible → Import translation** to browse the catalog from
[nani-samireddy/Holy-Bible-XML-Format](https://github.com/nani-samireddy/Holy-Bible-XML-Format).
Search by language or version, select a file, and choose **Download & import**. The catalog currently
lists 1,048 XML files. A GitHub `blob/master` link or corresponding raw XML link from that repository
can also be pasted directly. **Import local file** accepts existing Selah JSON packages and XML.
After a successful import the dialog closes and the Bible library selects the imported translation.

The parser accepts the repository's `<bible><testament><book><chapter><verse>` schema and Zefania
`XMLBIBLE/BIBLEBOOK/CHAPTER/VERS` files. Standard book numbers 1–66 map to canonical book codes;
partial editions and New-Testament-only files retain their original book numbers. Empty verses
used for omissions are left as numbering gaps. Nonempty verse text and inline text remain Unicode.
Book names supplied in Zefania files are retained; otherwise the existing canonical names are used.
Unsupported book numbers, invalid numbering, duplicate rows and malformed XML fail without a partial
translation being left behind.

Download happens asynchronously, followed by a worker-thread SQLite transaction and search indexing.
Imports are limited to 100 MB. Existing translation IDs are never overwritten. Stable XML identities
come from the source filename, so a second import of the same file is reported as a duplicate.
Successful imports work offline and do not change prepared programs, editor slides, or live output.

Source titles, rights/info/license text and original-source links are retained when supplied. Missing
copyright or license information is recorded as unspecified; importing does not declare an edition
public domain. The application bundles only the repository catalog metadata, not its Bible texts.

Repository imports show download progress with transferred/total MB and a percentage when the
server supplies a total. Unknown-size transfers use an indeterminate bar. After download, the
indicator switches to “Importing and indexing…” until the transaction finishes. The indicator is
shown in both the import dialog and Bible workspace, so closing the dialog does not hide progress.
Progress updates use a dedicated signal and do not refresh passage queries on every network chunk.
