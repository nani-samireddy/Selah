# Library component styling

Libraries use a native Qt interpretation of the [Toolcraft components reference](https://toolcraft.sh/components). Reference measurements were checked against its public starter component styles. Library controls share a compact neutral surface, Inter typography, subtle borders, 6 px control corners and 8 px menu/segmented corners.

## Shared controls

`LibraryStyle.qml` centralizes the palette, 32 px control height, 24 px page inset and 8 px field gap. `LibraryField`, `LibraryCombo`, `LibraryButton`, `LibraryTextArea`, `LibrarySpinBox`, `LibraryCaption`, `LibraryTabs` and `LibraryEmptyState` provide the library surfaces. Inter is bundled under the SIL Open Font License; document fonts remain independent.

- Text fields use 10 px horizontal insets and 13 px UI text. Search fields reserve an icon gutter.
- Selects use one Lucide chevron and a styled popup with a current-value marker, highlighted options and keyboard focus. String and object models retain their existing `textRole` behavior.
- Segmented groups size to their actual options with a 4 px inset and 2 px segment gaps. Their external selection remains owned by the calling screen.
- Numeric controls share the text-field height, use compact minus/plus affordances and retain editable bounds/locale parsing.
- Lyric text retains a larger reading size with Noto fallback for Telugu/Devanagari. The surrounding song form scrolls on compact windows.
- Primary actions use the established light button treatment; secondary actions use restrained surfaces and hairline borders.

Songs adds clear section labels and a stable-width browser. Bible uses the same controls and monochrome verse selection. Media uses adaptive thumbnail columns, contained filters and a centered empty/search state. Templates uses the same tab/filter, list and action treatments.

`LibraryField` and `LibraryCombo` are also reused by a few Programs/settings forms; those controls inherit the same metrics. The canvas and slide renderer keep their existing styling and presentation fonts.

## Verification

Run the workspace smoke captures at normal and compact sizes, check the model suite and QML lint, then verify select popup navigation and numeric editing in the native app. The fixture imports the project-authored ambient SVG into the isolated UX profile so the Media view is checked with an actual reusable asset.

Reference source: [Toolcraft starter styles](https://github.com/pixel-point/toolcraft/blob/main/starter/src/styles.css), with input/select/button primitives in the same public starter tree. Application QML components are independently implemented.
