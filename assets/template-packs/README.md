# Selah template collection

`selected/` contains the 19 layouts approved in the local Canva reference review on
27 November 2026. Each layout is one editable 1920 × 1080 canvas in **Libraries → Templates**.
It does not create a presentation deck or modify an existing program.

| Category | Reference IDs |
| --- | --- |
| Lyrics | B03, C03, G03, G07 |
| Scripture | B06, D03, E02, E07, F07 |
| Parallel Bibles | E03, I08 |
| Welcome | A01, H01, K02 |
| Announcements | F02, K09 |
| Closing | G12 |
| Standby | E10, J01 |

Text layers are editable, not flattened into screenshots. Lyrics and Scripture use live content
bindings, shrink-to-fit, and the existing logical pagination. Parallel Bibles have independent
body, version, and copyright fields and a single shared reference. The original source's unequal
columns were adapted to provide balanced reading areas. J01 is currently an editable **static
poster** based on the video reference; it does not include the source's animation.

## Assets and release status

Backgrounds were extracted from the user's exported Canva PDFs. Source text and rasterized text
effects were removed while retaining the background images, cropping, and decorative shapes.
E03 reuses the E02 background to remove the source's baked list bullets. No generated image is used.
The original decoded images and reference PDFs remain in the ignored local `captures/` directory.

`selected/sources.json` records each project/page, source fonts, and provenance. These Canva-source
assets are **not covered by the repository's code or font licenses**. Their individual redistribution
rights have not been verified. The collection is a local review candidate: `releaseReady` remains
false until the applicable Canva/content rights are checked for shipping editable app templates.
See https://www.canva.com/policies/content-license-agreement/ .

`asset:selected/media/<filename>` resolves to a hash-named durable local file on startup. Missing or
damaged files are repaired, existing user edits are preserved, and program snapshots remain
independent. Meadow/Garden are no longer included in the resource bundle. Untouched old templates
are archived in `retired_templates`; edited old templates and their existing media remain intact.

## Offline fonts

The collection bundles OFL fonts from Google Fonts with individual license notices and pinned
source URLs in `assets/fonts/selah-collection/sources.json`.

| Source | Selah font |
| --- | --- |
| Cormorant Garamond | Cormorant Garamond |
| Antic Didone | Antic Didone |
| Bebas Neue | Bebas Neue |
| Lato | Lato |
| New Icon Condensed / Snell Roundhand | Cormorant Garamond / Allura |
| Open Sauce One | Public Sans |
| Darts | Patrick Hand |
| Road Race Stamp | Barlow Condensed |
| Hertical Sans Rough | Freckle Face |
| Akhand Telugu / other Telugu content | Noto Sans Telugu |

C03 uses the source's clean body-text treatment with Public Sans rather than embedding Micky Rinse.
No proprietary font files were extracted from the PDFs. Existing bundled Noto script fallbacks
remain available for Telugu and Devanagari.
