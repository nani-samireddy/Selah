# Editor design direction

The user's references are the supplied Modfy editor video and https://toolcraft.sh/components. Preserve the video's canvas-first composition and Toolcraft's restrained control styling.

## Navigation and workflow

Programs, Libraries, Design and Present are persistent top-level workspaces. Programs is the starting point; primary workspaces occupy the body instead of stacking over the editor as modal dialogs. Keep the floating canvas composition inside Design. Outputs and Remote remain global settings, and content selection from Programs uses contextual pickers. Preparation selects drafts; explicit present actions publish snapshots. See the [workspace guide](features/workspaces.md).

## Surface and hierarchy

- Fixed dark editor appearance in this prototype. Canvas background `#111111`, panels `#191919`, controls `#252525`, hairline borders `#343434` to `#414141`.
- Main text `#DDDDDD`, supporting labels `#929292`. Keep disabled controls visibly subdued. White is reserved for the main Preview action and active details.
- Use a muted lavender selection outline on the canvas. Avoid using saturated accent colors throughout the shell.
- Panels have 9 px corners, fields 5 px, buttons 6 px, subtle shadows, and no decorative gradients. Artwork may use rich color.
- Keep floating layers at the left, controls at the right, tools centered at the bottom, and a large canvas between them.
- Keep editing properties compact: thin sliders, small handles, paired numeric fields, segmented alignment, restrained switches, and collapsible sections with reset controls.

## Library controls

Libraries use a shared Toolcraft-inspired component layer with bundled Inter typography, 32 px inputs, compact segmented tabs, single-chevron selects and consistent page/form insets. See the [library styling guide](features/library-ui.md). Slide typography and the floating editor remain independent.

## Type and assets

- Interface type uses Arial as a cross-platform starting point, 11–12 px for controls, 10 px for secondary labels. Increase contrast before making labels smaller.
- Slide typography is independent from interface typography. The initial artwork pairs an editorial serif headline with quiet sans-serif details.
- Use one consistent SVG icon family (Lucide), with explicit tooltips and accessible names on icon-only actions.
- Treat Telugu, Devanagari, other Indian scripts, and English as first-class document content. Bundle and test appropriate fonts before promising portable rendering.

## Interaction contract

- Object moves/resizes remain local during the gesture and enter history once on release; avoid rebuilding the document model for every pointer movement.
- Commit text on focus change or Escape. Draft edits must not silently update output previews.
- Undo/redo, focus rings, hover/down states, autosave status, and file errors are visible, functioning states.
- Pan and zoom do not mutate document coordinates. Fit restores the view without modifying slides.
- A clean output window receives a snapshot of the same logical slide elements. Future audience/stage/NDI paths must share the document model and rendering rules.

## Next quality gates

1. Validate the editor with keyboard, mouse, touchpad, and Indian-language IMEs on both Windows and macOS.
2. Add selection/movement keyboard shortcuts, richer resize handles, alignment guides, and rotation gestures.
3. Shared rendering, image/video layers, a reusable media library, and audience/stage routing are implemented. Sequential audience/stage fullscreen and concurrent offscreen capture passed on this Mac. NDI offscreen capture now passes video/straight-alpha checks; install the official SDK/runtime to compile and validate the network sender. Physical hot-plug and Windows remain untested.
4. Benchmark multiple outputs plus video on modest hardware.
5. Refine accessibility, font portability, reduced-motion behavior, and native application menus before release.
