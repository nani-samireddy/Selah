# Selah accessibility acceptance — macOS and Windows

Prepared 26 November 2026. Status: audit planned, not certified. Apply to the exact
installed release candidate in both themes. Record pass/fail, platform, reproduction
steps and evidence in the [RC record](release-candidate-record.md).

Use WCAG 2.2 AA as an engineering baseline where applicable to native UI; it is a
web standard and this checklist is not a claim of WCAG or jurisdictional compliance.
Use native accessibility APIs and actual assistive technology to assess the desktop app.

## Existing foundations and gaps

`StudioButton.qml` has strong keyboard focus, an accessible name and visual focus;
several fields/cards have explicit names and selection metadata. `InteractionFeedback.qml`
provides a saved reduced-motion option used by some animations. These are code observations,
not screen-reader test results. Verify every custom control, OS preference integration,
contrast over translucent panels and keyboard alternatives to canvas gestures.

## Keyboard and focus — release blocking

- [ ] Complete Programs → add content → reorder → preview → present → stop using only
      the keyboard. All actions, including icon buttons, split-button arrows, menus,
      cards, tabs, search results and dialogs, must be reachable and operable.
- [ ] Tab/Shift+Tab follow visible reading order; focus remains visible and unobscured
      by floating layers, inspector panels, compact slides or walkthrough overlays.
- [ ] Dialogs receive focus appropriately, contain traversal while modal and return
      focus to their trigger on close. Escape closes the intended surface without
      discarding work silently or unexpectedly changing live output.
- [ ] Arrow navigation works in Scripture, lists, segmented controls and grids; text
      fields retain normal editing keys. Merely browsing a verse never takes it live.
- [ ] Cmd/Ctrl+Z and redo, number-field stepping, menu shortcuts and shortcut hints
      work without conflicting with text editing, IME composition or assistive technology.
- [ ] Provide keyboard alternatives to drag reorder, element move/resize, pinch zoom,
      pan and other pointer-only gestures. Numeric geometry fields must be reachable.
- [ ] Confirm no keyboard trap outside intentional modal containment; test collapsing
      panels and removing/reordering focused items without losing focus.

## VoiceOver and Narrator — release blocking

- [ ] On macOS, complete core tasks with VoiceOver; inspect the native accessibility
      tree using Accessibility Inspector. On Windows, use Narrator and Accessibility
      Insights for Windows. A web accessibility scanner cannot certify native QML.
- [ ] Every interactive control exposes a useful name, role and applicable value,
      checked, selected, expanded and disabled state. Icon tooltips alone are insufficient.
- [ ] Stateful controls announce current action/state: preview open/closed, output
      stopped/fullscreen/blackout, audio muted, live slide and collapsed slide groups.
- [ ] Inputs have persistent labels; placeholders are supplementary. Units, bounds,
      validation errors and selected values are discoverable without color or hover.
- [ ] Search results, busy/progress/download completion and errors are discoverable
      without focus theft or continuous announcements. Live playback/timers must not
      flood speech; expose current state when requested.
- [ ] Canvas elements/layers expose useful names and selection, with accessible
      inspector alternatives. Decorative background dots/icons do not pollute navigation.
- [ ] Walkthroughs can start, advance, go back and exit with keyboard/screen reader;
      descriptions identify the control, rather than relying on position or highlight alone.
- [ ] Verify readable navigation for English/Telugu and mixed-script titles. Record
      speech-language limitations separately from missing text or accessibility metadata.

## Visual access and scaling — release blocking

- [ ] Measure text contrast: at least 4.5:1 for normal text and 3:1 for large text
      using WCAG definitions. Meaningful control boundaries, icons and state indicators
      meet 3:1 against adjacent colors where required. Check both themes and states.
- [ ] Test floating glass panels over light, dark and busy media; blur/transparency
      must not compromise legibility. Provide an opaque/high-contrast alternative if needed.
- [ ] LIVE, errors, selected cards and disabled actions do not depend on color alone;
      pair state color with text, shape, icon or accessible state.
- [ ] Test OS text/accessibility settings and magnification; text remains legible
      without clipping essential actions. Cover Windows 100/125/150/200% and mixed DPI,
      macOS Retina/scaled modes and the documented minimum usable window size.
- [ ] Test Windows contrast themes and macOS Increase Contrast/Reduce Transparency.
      Verify custom QML colors respond or provide an equivalent discoverable app setting.
- [ ] Aim for at least 24×24 logical-pixel hit regions as the native product baseline;
      use larger targets for primary live controls. Small visible icons can have larger
      non-overlapping hit areas. Document justified exceptions and test accurate activation.

## Motion, audio and presentation content

- [ ] Respect OS reduced-motion preference or offer an accessible saved equivalent;
      verify all UI breathing, dock/genie, pulse and zoom animations honor it.
- [ ] Keep operator UI reduced motion separate from authored audience transitions;
      document controls for each. Test non-animated alternatives and avoid hazardous flashing.
- [ ] Sounds can be disabled; no essential status is sound-only. Operator feedback
      must not leak into audience/stage output or interfere with screen-reader speech.
- [ ] Stop/pause controls remain discoverable during video/audio/standby playback.
      Test independent audience, stage and operator audio behavior.
- [ ] Document accessible template authoring: readable size/contrast, safe text fit,
      language/font fallback, captioned media where available, and avoiding text baked
      into images. Audit built-in templates separately from arbitrary imported user content.
- [ ] Check LAN remote separately: keyboard, screen reader, touch targets, zoom,
      labels and reconnect/error states on supported phone/tablet browsers.

## Acceptance scenarios

| Scenario | macOS evidence | Windows evidence |
| --- | --- | --- |
| Create/open program; add song, Scripture and media; reorder | Pending | Pending |
| Edit text, choose font/color, resize without dragging; undo/redo | Pending | Pending |
| Search Scripture, select verse, browse arrows; explicitly present | Pending | Pending |
| Select output, open/close preview, blackout/restore, stop presenting | Pending | Pending |
| Import/download error, cancellation, backup/restore and recovery confirmation | Pending | Pending |
| Navigate and exit guide and Luma dialogs | Pending | Pending |

A task is blocked if the operator cannot complete it independently, loses content,
or changes live output unintentionally. Assign a severity/owner to every failure;
retest fixes with the same assistive technology. Do not mark accessibility complete
from static QML inspection or automated scanning alone.

## Primary references

- [Qt Quick accessibility](https://doc.qt.io/qt-6/accessible-qtquick.html): native control behavior and Accessible metadata.
- [Microsoft accessibility checklist](https://learn.microsoft.com/en-us/windows/apps/design/accessibility/accessibility-checklist)
  and [testing](https://learn.microsoft.com/en-us/windows/apps/design/accessibility/accessibility-testing).
- [WCAG 2.2](https://www.w3.org/TR/WCAG22/): engineering baseline for keyboard,
  contrast, focus, target size and motion; apply with native-platform judgment.
