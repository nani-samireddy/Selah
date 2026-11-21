# Appearance

The workspace toolbar has a sun/moon button that toggles **Light** and **Dark**. The icon changes to show the next action. Right-click or hold the button to choose **System**, **Light**, or **Dark**. The choice is stored in the local `Appearance` settings category and restored on launch. System follows the operating system's color scheme.

`LibraryStyle` owns appearance preferences and semantic neutral colors. `tone()` adapts the established interface's neutral hierarchy; colored status badges keep their own contrasting foreground/background pairs. Panels, libraries, editor controls, menus, dialogs, typography, and icons use the same appearance source. The editor grid repaints when appearance changes.

The toolbar uses light logo ink on dark surfaces and dark ink on light surfaces. SVG branding assets are kept apart from the Lucide control icons. Slides, media, templates, presentation output, and NDI content retain authored colors regardless of interface appearance.

## Interaction feedback

Buttons use a small 220 ms press/release response with restrained easing; hover and selected colors fade over 160 ms. Dropdowns fade and settle in while their chevrons rotate. Successful layer drops have a selection cue. These effects change only operator controls, never authored slide geometry, output timing, or live content.

The speaker icon toggles interface sounds. Right-click or long-press it for Interface sounds and Reduced motion. Both preferences persist. Reduced motion disables these transitions and scale effects. Interface sounds are short local cues, rate limited and preloaded; hovering and typing remain silent. Present workspace automatically silences interface cues. Media audio keeps its independent playback behavior.

Reference: [Shaper by UI8](https://shaper.ui8.dev/) inspired the timing and tactile control response, without copying its sound assets.

## Compact typography

The operator controls use bundled Inter consistently, with 28 px standard heights, 13 px regular button labels with medium primary actions, 12 px field text, and 12 px medium captions. These measurements follow the [Toolcraft Mesh FX demo](https://toolcraft.sh/demos/mesh-fx). Shared operator text controls use Qt text rendering to avoid the overly heavy appearance of native-rendered variable Inter on macOS. Authored slide text and its renderer retain their own typography and rendering behavior. Field gaps are 6 px and section gaps are 16 px.

The interface click and selection cues use the supplied `click-004.mp3`, decoded into the bundled `click.wav`. Existing sound volume, mute preferences and Present-mode silence still apply.
