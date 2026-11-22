# Reference template packs

Meadow and Garden are individual 1920 × 1080 templates installed in **Libraries → Templates**.
They do not create Custom Slides entries or change a saved program/editor draft.

- **Meadow:** 11 editable presentation layouts and lyrics, bilingual and scripture variants.
- **Garden:** 9 editable presentation layouts and lyrics, bilingual and scripture variants.
- Each JSON manifest lists stable template identities and the photographs exposed in Media.
- Text layers remain editable. Illustrations, rules and panels remain separate image/vector layers.
- `asset:<pack>/media/<filename>` is resolved by `TemplatePresets` to a durable local file before
  validation. The application never stores a developer-machine path in bundled definitions.
- On startup, missing or damaged bundled assets are restored. Existing templates, including user
  edits and revision numbers, are preserved. Programs retain their independent snapshots.

## Reference and asset provenance

Source: the user's Canva design, https://www.canva.com/design/DAHbkiFibr0/edit, inspected in Chrome.
Garden images were extracted from the user's 12-page PowerPoint export; Meadow images and
illustrations from the later 11-page export. They retain their Canva-source licensing and are
not covered by the repository's code or font licenses. The user/account's applicable Canva rights
must cover any redistribution of those assets.

`meadow/media/meadow-wide.jpg` is a widescreen crop of the exported clouds/meadow photograph.
Other photos and illustrations are original exported media. Small SVG rules/panels are authored
for these layouts. Sample wording replaces the source's placeholder text and fictional church
identity. These are reference-inspired layouts, not a general PowerPoint importer.

## Fonts

Garden uses Cardo Regular/Bold and Prompt Bold, matching the reference. Meadow uses Public Sans
for body text. Hertical Rough, the source's heading font, is commercially licensed and is not
included; Freckle Face provides an editable, freely distributable display alternative. These
Google Fonts are bundled for offline use with their individual OFL notices in `assets/fonts`.
