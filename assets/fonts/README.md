# Bundled fonts

- `Inter.ttf`: variable Inter from the [Google Fonts Inter source](https://github.com/google/fonts/tree/main/ofl/inter), used by the library interface. Keep `Inter-OFL.txt` with the font.
- `NotoSansTelugu.ttf` and `NotoSansDevanagari.ttf`: document/lyrics fonts with the existing SIL Open Font License notices.

The fonts register in `ApplicationSetup` before QML loads. UI typography stays separate from presentation font choices; changing library controls must not rewrite a slide's font family. Keep fonts bundled so Windows and macOS use consistent metrics.

The reference template packs also bundle Cardo Regular/Bold, Prompt Bold, Public Sans (variable),
and Freckle Face. Each family has its original `OFL.txt` beside the font files. They register at
startup without installing system fonts. Freckle Face substitutes for the commercially licensed
Hertical Rough heading font in the Meadow reference.
