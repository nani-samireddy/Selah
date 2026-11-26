# Shared multilingual reference matching

`BookMatcher` maps an explicit reference to a book identity and numeric range. It
has no database or output-writing access. `ContentLibrary::resolveReference`
combines names from all installed Bibles with the built-in catalog, then checks
availability in the selected translation. Both `searchVerses` and Luma use this
same path. `AiRequest::scripture` remains a compatibility adapter for pure tests.

Resolution tries native/known names and generated Romanized names first, then
bounded spelling matches before the numeric reference. Numbered-book prefixes
remain distinct. The result contains either a canonical query, a clarification
error and candidate choices, or `handled=false` for ordinary word searches.
Chapter and verse numbers are normalized but never inferred or corrected.

The Bible browser displays ambiguous candidates as explicit choices. Luma asks
for a clearer name before proposing a search. Neither path presents content or
changes a saved program. See `assets/book-names/README.md` for language coverage
and `tests/unit/BookMatcherTests.cpp` for matching and ambiguity checks.
