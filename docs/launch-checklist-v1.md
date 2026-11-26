# Selah v1 launch checklist — macOS and Windows

Updated 26 November 2026. This is a release gate, not a claim that all features
are production verified. Check an item only when its evidence is recorded for
the exact release candidate. Linux packaging is outside this first-launch gate.

## Current launch decision: NO-GO pending evidence

The checkboxes below are unverified release gates, not a completed-feature list.
A development build or passing model test does not certify a shipped package.
For each gate, attach an issue or evidence record and assign a named owner.

| Priority | Workstream | Repository evidence / remaining gate | Owner |
| --- | --- | --- | --- |
| P0 | macOS distribution | Packaging is ad-hoc signed; Developer ID, hardened runtime, notarization and clean-Mac installation remain | Release owner: assign |
| P0 | Windows distribution | Packaging is unsigned ZIP; choose installer versus portable release, sign and rehearse install/upgrade/uninstall | Release owner: assign |
| P0 | Accessibility | Some QML names, focus and reduced-motion hooks exist; full keyboard, VoiceOver and Narrator acceptance is unverified | Accessibility owner: assign |
| P0 | Data and live safety | Recovery, backups, bundles and cues need packaged cross-platform rehearsals | QA owner: assign |
| P0 | Distribution rights | Audit exact Qt modules, media codecs, Bibles, fonts, sounds and model artifacts before bundling | Release owner: assign |
| P0 | Updates | Manual download flow exists; exercise a real signed stable release and upgrade path | Release owner: assign |
| P0 | Test health | Resolve remaining standby-media model-test setup failure and verify rendered video pixels; do not count frame delivery alone as visual proof | Engineering owner: assign |
| P1 | Luma / NDI | Complete advertised-platform checks, or clearly exclude unavailable capabilities from v1 claims | Engineering owner: assign |

P0 means block public release. P1 means fix or explicitly defer the feature before
scope freeze; it cannot be advertised as verified. No checks have been signed off
by preparing this document.

Use the [accessibility acceptance checklist](accessibility-checklist.md) and copy
[the release candidate evidence form](release-candidate-record.md) for each RC.

## Release definition

- [ ] Freeze the v1 feature list and release version/build number (CMake currently
      declares 0.1.0). Publish the same version in the app, packages and release notes.
- [ ] Confirm supported OS versions from the deployed Qt binaries and clean-machine
      testing. Record minimum macOS and Windows versions in the download page.
- [ ] Confirm architecture labels: initial candidates are macOS Apple Silicon and
      Windows x64. Do not advertise Intel Mac or Windows ARM without their own builds/tests.
- [ ] Decide whether NDI is in v1. If shipped, complete the sender checks below;
      otherwise explain its availability accurately and remove prototype wording
      from features advertised as finished.
- [ ] Keep AI optional and off by default. The new backend is preparation work:
      in-app managed downloads and narrow English/Telugu/mixed-language app checks are implemented;
      broader model-language accuracy and clean Windows validation remain pending.
- [ ] Validate Luma with English, Telugu, code-switching and ambiguous/spelling-heavy
      commands against saved content; keep operator review before every action.
- [ ] Validate Luma installation and inference on a clean Windows x64 machine.
- [ ] Validate the implemented manual stable-release check/download flow against an actual
      published release on both platforms. Automatic installation still requires the
      signed native adapters in the [update plan](release-updates.md).

## Multilingual matching backlog

- [ ] Expand shared Bible book-name matching in Bible search, Scripture mode and
      Luma beyond the initial English, Telugu and Hindi catalogs. Cover the languages
      represented by the 1,048-edition import catalog in stages, with tested coverage
      recorded per language rather than assuming every imported edition is supported.
- [ ] Normalize the catalog's 244 language labels to verified language identities;
      correct spelling variants and ambiguous regional labels such as Kenya, Papua
      and Original before using them to measure coverage.
- [ ] Add native book names, common abbreviations and numbered-book forms; extend
      Romanized/transliteration matching for additional scripts and mixed-language
      requests, independently of the selected Bible's language.
- [ ] Test native and Romanized spellings, typing mistakes and ambiguous names for
      each supported language. Preserve explicit chapter/verse numbers, retrieve only
      saved scripture, and require clarification when a match is uncertain. Verify
      the same behavior in Bible search, Scripture mode and Luma on macOS and Windows.

## Known packaging blockers

Current repository evidence: `scripts/package-macos.py` creates an ad-hoc-signed,
non-notarized bundle; `scripts/package-desktop.py` labels its Windows package
unsigned. `.github/workflows/build-desktop.yml` uploads temporary Windows/Linux
artifacts, not permanent end-user releases. These scripts exist, but that does
not certify a production artifact.

- [ ] Configure Developer ID signing for macOS; sign nested frameworks/helpers,
      notarize the distributed artifact, staple where applicable and verify it on
      a clean Mac without Qt/Homebrew or developer tools.
- [ ] Choose the Windows distribution form (installer or explicitly portable ZIP).
      Set icon/version/publisher metadata and sign the release executable/installer.
- [ ] Install on a clean Windows x64 machine without Qt, Visual Studio or a developer
      PATH. Verify app-local runtime DLLs, SQLite, media plugins and QML deployment.
- [ ] Produce the final macOS artifact from `scripts/package-macos.py` and Windows
      artifact from `scripts/package-desktop.py` after signing support is configured.
      Keep developer tests, fixtures and diagnostics out of both production packages.
- [ ] Publish permanent, versioned release assets, SHA-256 checksums and concise
      release notes. Test downloading those exact files on another computer.

## Shared functional gates

- [ ] Run formatting, QML lint, model checks and relevant GUI smoke checks on the
      release commit. Save logs with commit SHA, Qt version and artifact checksums.
- [ ] Complete a realistic Sunday-service rehearsal: songs, scripture, media and
      saved presentations in one program, with reordering, template changes,
      transitions, standby media, blackout and an obvious Stop presenting action.
- [ ] Rehearse Quick Add during an ongoing service: preview without changing live
      output, Insert next after a whole song, spontaneous lyrics/text, guest media,
      Present now and Resume program. Verify draft recovery and explicit library saving.
- [ ] Verify import/export, duplicate names, empty libraries, missing media and
      restarting after unsaved changes. Verify a real backup/restore cycle.
- [ ] Verify an existing user's data opens after upgrading; retain the current
      ChurchDeck storage namespace. Document backup, downgrade and recovery limits.
- [ ] Test English/Telugu plus the additional Indian languages advertised for v1:
      shaping, fallback fonts, line splitting, mixed scripts, bilingual lyrics and
      two Bibles at the same reference. Test downloaded fonts and Bibles offline.
- [ ] Review Bible/font/media/dependency notices and permissions for anything bundled.
      Import capability is not permission to redistribute every source translation.
- [ ] Test supported image/video/audio formats from the packaged apps, including
      missing files, long playback and muted operator feedback during Present.
- [ ] Verify Undo/Redo, text selection/formatting, resize handles, pinch/scroll/pan,
      compact slides, color/alpha inputs and light/dark themes at supported window sizes.
- [ ] Test keyboard focus, readable contrast, accessible control names, reduced
      motion and the interactive guides with a first-time operator.

## macOS hardware gates

- [ ] Test the minimum supported macOS release and a current release on real
      Apple Silicon hardware; verify microphone/local-network prompts only when used.
- [ ] Rehearse with two physical external outputs where available: audience and
      stage routing, windowed preview, fullscreen, unplug/reconnect and operator-screen protection.
- [ ] Verify traffic-light buttons, window dragging/resizing, native menus, Cmd+Z,
      Shift+Cmd+Z and trackpad gestures from the installed bundle.
- [ ] Verify opening exported projects/media from paths with spaces and non-Latin characters.

## Windows hardware gates

- [ ] Test the supported Windows versions, including a standard non-admin account.
- [ ] Verify 100%, 125%, 150% and 200% display scaling, mixed-DPI monitors and
      moving the operator window between monitors.
- [ ] Rehearse audience/stage fullscreen routing, projector hot-plug, sleep/wake,
      window controls, keyboard shortcuts and trackpad/mouse input on Windows hardware.
- [ ] Verify playback and Google Font loading from the packaged app, firewall
      behavior for the optional LAN remote, and Unicode file/user-profile paths.

## Remote and NDI gates

- [ ] Pair a real phone/tablet on Wi-Fi using the advertised LAN address; test
      reconnects, incorrect pairing, stopping the server and the documented access roles.
- [ ] If advertising NDI, package the required sender support/runtime and verify
      actual network video in an independent NDI receiver on both OSes. Capture-only
      diagnostics do not prove network transmission; SDK-missing builds disable it.
- [ ] Verify NDI transparency for lower thirds and overlays, transitions, media,
      clear text, blackout, standby media and stopping while sender startup is pending.
- [ ] Run a service-length soak test with physical outputs and NDI as advertised;
      record frame pacing, memory, media sync and crash behavior.

## Release sign-off

- [ ] Have a new operator complete the getting-started walkthrough without developer help.
- [ ] Publish installation, offline preparation, troubleshooting, backup and support instructions.
- [ ] Record all known issues and a rollback/recovery procedure; resolve launch-blocking failures.
- [ ] Approve the exact signed artifacts after the final rehearsal. Publish from the
      verified release commit; do not replace an existing version's binaries silently.

Evidence record for each platform: commit / version / Qt version / OS / CPU and GPU /
display setup / tests and rehearsal date / artifact SHA-256 / reviewer.

## Service tools release rehearsal

- [ ] Rehearse chords with real musicians: Telugu/English lyrics, repeated sections, long wrapped lines and stage-only cues. Verify audience/NDI stay free of chord tokens.
- [ ] Force an unexpected exit in an isolated profile with spontaneous Scripture and Quick Add content; restore only by confirmation and verify outputs/audio remain off.
- [ ] Transfer a portable program bundle between the packaged macOS and Windows apps with the original media unavailable. Check fonts, backgrounds, video playback and templates offline.
- [ ] Export and restore a full app backup on each OS; verify Bibles, libraries, preferences, media path relocation, cancellation, damaged-file rejection and the preserved previous profile.
- [ ] Review service cue presets before rehearsal; verify missing prerequisites apply no partial settings and stopped outputs remain off.

## Privacy, security and distribution rights

- [ ] Publish a plain-language privacy notice describing local storage and every
      optional connection: Bible/font/model downloads, update checks, LAN remote,
      HTTP/OSC cues and any enabled AI service. Verify actual endpoints and data sent;
      do not claim all processing is offline without checking the packaged backend.
- [ ] Keep credentials, signing keys, pairing tokens and personal service content
      out of packages, logs and release artifacts. Document support-log redaction.
- [ ] Verify LAN pairing, role permissions, revoke/stop behavior and the trusted-network
      limitation. Test cue network actions only against explicit operator configuration.
- [ ] Audit archive and import limits, path traversal, malformed inputs, low disk space,
      cancellation and partial writes. Verify originals survive failed restore/update.
- [ ] Inventory exact runtime dependencies and licenses, including Qt modules/private
      APIs, FFmpeg/codecs, NDI if enabled, Cactus/model weights, fonts, icons and sound.
      Choose a compatible distribution license for every shipped component; dynamic
      Qt linking alone does not settle all module/license obligations.
- [ ] Include third-party notices, required license texts and required source/relinking
      materials or offers for the chosen licenses. Verify redistribution rights for
      each bundled Bible and media asset separately from import support.
- [ ] Decide distribution territories, store versus direct download, commercial versus
      free terms and applicable privacy/accessibility obligations before publication.
      The accessibility baseline is a product gate, not a legal compliance certificate.

## Installer and upgrade acceptance

- [ ] macOS: verify Developer ID identity, secure timestamp, hardened runtime and
      minimal entitlements; sign all nested code, notarize final distribution, staple
      where supported and test Gatekeeper on a browser-downloaded artifact.
- [ ] Windows: choose signed EXE/MSI/MSIX installer or an explicitly supported portable
      ZIP. If installer: shortcuts, install scope, cancel, repair and uninstall must
      work as documented; preserve user libraries unless deletion is explicitly chosen.
      If portable: document extraction, launch and replacement without developer tools.
- [ ] Validate Windows publisher/signature and timestamp on the final executable and
      installer as applicable. Do not promise signing eliminates SmartScreen warnings.
- [ ] Exercise update available / no update / offline / cancelled / invalid digest /
      unsupported architecture. Prevent install or replacement while outputs are live.
      For v1, document manual replacement; automatic install/restart remains future work.
- [ ] Upgrade from the prior public or seeded legacy profile with actual content;
      verify ChurchDeck paths, settings, media and DB migrations. Test backup restoration
      after a failed upgrade. Do not assume older binaries can open migrated data.
- [ ] Download and install as a normal user on clean supported OS images; record
      security prompts, runtime prerequisites, disk space and first-launch behavior.

## Release execution order

1. Assign owners, freeze v1 scope and confirm OS/architecture support from deployed binaries.
2. Resolve P0 implementation gaps; finish accessibility and dependency audits.
3. Build one immutable RC from a recorded commit and toolchain; retain symbols privately.
4. Run automated checks, clean installs, accessibility tasks and a full service rehearsal
   against those packages. Save evidence using the RC form, including failed attempts.
5. Sign/notarize final artifacts and rerun installation/security and critical workflow
   checks on the exact distributable bytes; record their SHA-256 values.
6. Approve release notes, notices, download/help pages and support/rollback instructions.
7. Publish versioned assets, verify the public download/update path, and retain the prior
   release. Any binary change creates a new candidate/version and requires new evidence.

## Reference sources

- [Apple: Developer ID](https://developer.apple.com/developer-id/) and
  [notarization](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution/).
- [Microsoft: code signing choices](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options).
- [Qt licensing by module](https://doc.qt.io/qt-6/licensing.html) and
  [GPL/LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations).
- [Accessibility acceptance and primary references](accessibility-checklist.md).
