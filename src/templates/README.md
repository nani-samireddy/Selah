# Reusable content templates

- `TemplateStore`: local catalog, starter layouts, revisioned saves and atomic exchange files.
- `TemplateDefinition`: supported kinds/bindings, geometry validation and media availability.
- `TemplateTextLayout`: shaped text measurement and grapheme-safe page boundaries.
- `TemplateComposer`: lyrics/scripture pagination and binding population without changing the saved design.

SlideFactory delegates scripture to the composer for automatic and reusable layouts; automatic lyrics retain their row-based generator. ProgramStore stores copied definitions and source content; editing a catalog layout never changes a prepared or live service automatically.

Do not replace shaping with character-count estimates. Fallback font metrics, actual box geometry and each binding's multiple visible boxes all matter. Body fonts follow their selected sizing mode; metadata can shrink when necessary. Invalid or impossible layouts fail before replacing active slides.

See [the feature guide](../../docs/features/templates.md), [architecture](../../docs/architecture.md) and `TemplateStoreTests` / `TemplateChecks` under tests.

`TemplatePresets` installs the resource-backed Meadow/Garden layouts using stable preset IDs and
content-hashed local media. It preserves saved edits and does not own editor/program state. Pack
manifests and source/font provenance live in `assets/template-packs`. Presentation-kind templates
have static editable text and are deliberately excluded from automatic lyrics/scripture generation.

Scripture pagination chooses paragraph/sentence/clause boundaries, preserving
all source characters. Oversized phrases fail with an actionable error rather
than using an arbitrary word split. Lyric rows retain their existing word-based
fallback. Automatic scripture generation also uses this composer. Each box can
select `textSizing: shrink` with a bounded `minFontSize`; populated snapshots use
measured fitted fonts without modifying the template definition.
