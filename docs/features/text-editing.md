# Text editing

Double-click a text layer in Design to edit it. Select words to format only that range; inspector changes on a selected layer apply to the whole layer. Escape finishes text editing. Bold, italic, and underline use the standard platform keyboard shortcuts.

The typography inspector includes Google/system font selection, weight, size, italic, underline, strikethrough, text color, horizontal alignment including justification, vertical alignment, letter spacing, and line height in percent or pixels. Text boxes can keep fixed bounds or grow automatically in width or height.

**More text options** exposes text case, small caps, paragraph spacing, first-line indentation, numbered and bulleted lists, list indentation, superscript/subscript, wrapping, clipping, maximum displayed lines, ellipsis, common OpenType features, and variable font axes. Four-character custom OpenType tags are accepted. A font must actually provide a feature/axis for that control to change its rendering.

Text remains available as plain Unicode for search, remote summaries, and content bindings. Rich character/paragraph formatting is stored separately; features that Qt's HTML serialization cannot preserve are retained as typed character ranges. Projects, program snapshots, editor previews, audience/stage slide renderers, and NDI share this representation. Text replaced through a template binding discards incompatible old character ranges. Theme changes affect the application interface, never authored text colors.

`TextFormatting` is a small registered QML module backed by the shared C++ document helper. Resource loading is disabled while parsing rich text; links and object/resource formats are omitted when rebuilding its safe representation. Rendering truncation leaves authored content intact.

This provides Figma-style text controls in the native editor. Font rendering and available OpenType behavior follow Qt and the installed font; this is not an assertion of pixel-for-pixel Figma text layout compatibility.

Template pagination measures through the shared rich-text document layout, including line height, letter spacing, font weight, and variable font settings. A full text selection updates the layer defaults that bound content inherits; placeholder character ranges are cleared when dynamic content replaces them. Omitted line-height settings retain native 100% spacing for older slides.
