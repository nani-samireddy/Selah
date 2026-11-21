# Google Fonts catalog

`catalog.json` is a metadata snapshot of all 1,961 families listed by the official `https://fonts.google.com/metadata/fonts` endpoint when generated. It references native font files and license notices from the pinned `google/fonts` source commit recorded in the JSON. Families not matched to that source snapshot use Google's official CSS/native-TTF endpoint on demand.

`OFL.txt` contains the standard SIL Open Font License text, used together with the downloaded font's embedded copyright/license notices when needed. Original upstream license files are retained for repository downloads. The app exposes downloaded notices through the font picker.

Refresh with `python3 scripts/build-google-font-catalog.py`. The script rejects truncated source trees. It downloads metadata only; selecting a font in the app downloads and caches that family, leaving the installer small.
