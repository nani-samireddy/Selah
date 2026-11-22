# Reusable content templates

- `TemplateStore`: local catalog, starter layouts, revisioned saves and atomic exchange files.
- `TemplateDefinition`: supported kinds/bindings, geometry validation and media availability.
- `TemplateTextLayout`: shaped text measurement and grapheme-safe page boundaries.
- `TemplateComposer`: lyrics/scripture pagination and binding population without changing the saved design.

SlideFactory delegates to the composer only when a reusable definition is explicitly chosen. Automatic layout keeps the earlier generator for compatibility. ProgramStore stores copied definitions and source content; editing a catalog layout never changes a prepared or live service automatically.

Do not replace shaping with character-count estimates. Fallback font metrics, actual box geometry and each binding's multiple visible boxes all matter. Body fonts remain fixed; metadata can shrink when necessary. Invalid or impossible layouts fail before replacing active slides.

See [the feature guide](../../docs/features/templates.md), [architecture](../../docs/architecture.md) and `TemplateStoreTests` / `TemplateChecks` under tests.

`TemplatePresets` installs the resource-backed Meadow/Garden layouts using stable preset IDs and
content-hashed local media. It preserves saved edits and does not own editor/program state. Pack
manifests and source/font provenance live in `assets/template-packs`. Presentation-kind templates
have static editable text and are deliberately excluded from automatic lyrics/scripture generation.
