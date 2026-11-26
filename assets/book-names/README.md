# Multilingual Bible book names

`catalog.json` links built-in native names to stable USFM book codes. Initial
catalogs contain Telugu and Hindi names; English names and localized names from
all imported translations join the index at runtime. Matching is independent of
the language of the Bible whose verse text will be returned. Add language data
here without changing model weights or adding one-off Latin spellings in C++.

`indic-romanization.json` maps Unicode characters to orthographic Romanization
components. It was generated from Python's Unicode 14.0 character names for
Devanagari, Bengali, Gurmukhi, Gujarati, Odia, Tamil, Telugu, Kannada and Malayalam.
The matcher derives candidates from native book names, including vowel signs,
virama, nasal marks, long-vowel spelling variants and optional final inherent
vowels. This is an orthographic candidate generator, not a general translation
model or a claim of pronunciation accuracy in every language.

Romanized names need a corresponding native or Latin name in the built-in or
imported catalog. Having a Bible edition does not itself establish name coverage
when that edition's metadata supplies only English book names. More native-name
catalogs still need to be added and tested. Unknown scripts retain native exact
matching, and imported Latin names support accent normalization.

Test every new catalog using native and Romanized spellings, numbered books,
misspellings and collisions. Bounded fuzzy matching resolves a unique nearest
candidate; tied matches must return choices, never an arbitrary book. The
selected translation must contain the resolved book and requested verse.
