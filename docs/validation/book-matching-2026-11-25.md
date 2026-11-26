# Shared multilingual matching — 25 November 2026

macOS ARM64 build/model checks and QML lint passed. Thirteen Luma integration
scenarios passed using the actual local runtime for free-form English and the
shared reference matcher for explicit native/Romanized requests. Each approved
reference was compared with exact saved Bible rows. Draft, catalog, program and
live snapshot remained unchanged. The adjacent JSON records the routing source.

Verified against the selected Telugu Bible: `adikandam 1:1`, `aadikandamu 1:1`,
`ādikāṇḍam ౧:౧`, `yohanu 3:16`, `utpatti 1:1`, `उत्पत्ति १:१`,
`yuhanna 3:16`, mixed-language commands and numbered `1 yohanu 3:16`.
Tied synthetic book names produced multiple explicit choices. Missing books,
missing chapters, invalid ranges and multiple references were not guessed.

The GUI check entered `adikandam 1:1` in the Telugu Bible search, compared the
returned verse with Genesis 1:1, selected it and sent a real right-arrow key
event. It continued to verse 2 without taking either verse live.

Built-in native catalogs cover Telugu and Hindi; English and localized imported
names join the index at runtime. The orthographic candidate generator supports
characters from nine Indic script blocks. This does not certify every language,
book-name convention or Roman spelling. Windows execution validation and more
native-name catalogs remain pending.
