# Reusable content templates

Templates connect the canvas editor to song and Bible content. A template is a named, typed slide layout with bound text boxes, static layers and optional media. Text generation follows the template's geometry and fonts; it does not paste an old slide into a differently sized box.

## Create and edit

1. Open **Libraries → Templates**. Three starter layouts are included: Lyrics, Bilingual lyrics and Scripture.
2. Choose a layout and **Edit layout**, or choose **New template → Create & edit**. Opening a layout replaces the editor draft as one undoable operation and enters Template mode.
3. Resize/move/style text boxes on the canvas. In **Properties → Content binding**, assign each dynamic text box a source. Unbound (`None`) layers remain static.
4. Choose **Save template** in the canvas toolbar (or Cmd/Ctrl+S in Template mode). **Save new** creates an independent layout. **Update template** creates a new revision of the opened layout.
5. Use **Templates** to preview with sample content. Return to Programs to preview the actual service content before applying.

The existing editor recovery file still autosaves the draft. Saving a template is explicit and stores it in `templates.sqlite`; saving the draft alone does not update the reusable catalog.

## Kinds and bindings

| Template kind | Required visible bindings | Other available bindings |
| --- | --- | --- |
| Lyrics (`song`) | Primary lyrics | Title, Section name, Credits |
| Bilingual lyrics (`bilingual`) | Primary lyrics, Translated lyrics | Title, Section name, Credits |
| Scripture (`scripture`) | Scripture text | Title, Scripture reference, Version, Credits |

A song with translated rows requires a bilingual template. A bilingual template can also be used for a single-language song, leaving translated boxes empty. Multiple boxes can use the same binding; pagination must satisfy every visible box for that binding. Hidden boxes do not determine page capacity.

For songs, Title is the song title and Section name is the verse/chorus label. For scripture, Title and Scripture reference use the passage reference, Version uses its abbreviation and Credits uses its copyright notice. Source copyright/license metadata is retained in generated slides even if the layout omits its visible footer.

Keep visible bound boxes inside the 1920×1080 canvas, including rotated corners. Templates validate element data and binding compatibility before saving or applying. Use the existing font selector; bundled Telugu/Devanagari fonts and platform fallback fonts handle shaping.

## Generate and apply

Song and Bible libraries include a **Layout** selector. Automatic layout preserves the existing generator; choosing a saved layout generates slides from that design when adding to the editor or a program.

In **Programs**, select a song or Bible item and choose **Choose template**. The picker previews the actual content and lets you inspect every resulting slide. **Apply layout** replaces the draft item's slides as one undoable change. **Reset** regenerates the automatic layout. Existing palette styles remain available separately.

Program items store a complete copy of their chosen template definition and revision. Updating a template in the catalog does not change a prepared service. Reapply the layout explicitly to adopt the latest revision. Choosing another song arrangement uses that item's stored template copy. Editing generated slides does not edit the reusable template; reapplying a template or changing the arrangement regenerates those slides from the stored source content.

The live snapshot remains unchanged until **Take slide live**. Editing the catalog, applying a layout, undo/redo and previewing layouts never publish to audience/stage/NDI by themselves.

New Bible items retain complete verse-source snapshots. For older items, the app can recover prepared scripture text and attribution from identifiable bound slides, preserving their existing line breaks. If the source/reference cannot be recovered, application fails without replacing the item and asks you to re-add the passage from the Bible library.

## Splitting and fit

Text is measured with `QTextLayout` using each box's font, pixel size, width and height, including fallback glyph metrics. A small internal safety margin protects glyph edges. The Lines/slide control is an upper limit; physical box capacity may require fewer lines.

- Song sections follow the saved arrangement, including repeats. Blank paired rows force a new slide.
- Corresponding primary/translated rows are packed together when they fit. A long row continues across pages at safe grapheme/word boundaries before moving to the next row. Unequal translation lengths may leave one language box empty on continuation pages; no content is discarded.
- Each Bible verse starts a new slide; long verses have numbered continuations.
- Body font sizes remain as designed. Metadata such as titles, references and credits may shrink down to 8 px to fit their boxes. If even that cannot fit, application stops with an actionable error.
- Static text, image/video layers, colors, alignment, opacity and layout are retained. Text is rendered as plain text.

If a single text cluster cannot fit, enlarge the relevant box or reduce its font. The app refuses that generation rather than silently clipping or omitting content. Existing 500-slide-per-item and 2,000-slide-per-program limits apply.

## Import/export and storage

**Import/Export** uses `.churchtemplate` JSON with `format: "churchdeck-template-1"` and a `template` object containing `name`, `kind` and `slide`. Imports receive a new identity. Updates increase the existing template's revision. See [the import example](../../examples/templates/basic-lyrics.churchtemplate).

The slide uses the same element format as `.churchdeck` projects. Media URLs remain references to local files; export does not bundle media. The library and picker report missing media. Template files are local data, not executable plugins.

## Development checks

```sh
python3 scripts/format.py --check
cmake --build build --target church-deck_qmllint
ctest --test-dir build --output-on-failure
build/Selah.app/Contents/MacOS/Selah --fresh --compact --template-smoke "$PWD/captures/templates"
```

Model checks cover type/binding validation, text fit, lossless multilingual splitting, repeated arrangements, static layers, revision persistence, exchange, legacy passage recovery, program snapshots and undo. The native check previews service content, opens the editor, changes the layout, checks rendered text bounds/background updates and verifies live isolation. It uses the verification profile and original test lyrics.

Portable asset packaging, additional content bindings, variable canvas sizes, template deletion and Windows rendering validation remain separate work.
