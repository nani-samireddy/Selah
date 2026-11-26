# Selah v1 capability boundaries

Updated 26 November 2026. These are launch-scope statements; packaged Windows and
macOS acceptance remains governed by the release checklist and evidence records.

## Luma

Luma is an optional, local **library search assistant**, disabled until enabled by
the operator. The pinned native runtime and Needle model download separately
(about 37 MB); they are not included in the base installer. The operator reviews
proposals before search and explicitly selects content to present. Luma cannot
change live outputs or invent scripture through its current two search tools.

Initial evaluation covers English, selected Telugu commands and English/Telugu
mixed requests on Apple Silicon. Explicit references use the shared book matcher
before inference; Telugu/Hindi native catalogs and imported book names expand
reference lookup. Romanization rules alone do not establish coverage of every
Bible language. A catalog of 1,048 editions is not a claim of 1,048-language AI support.
Do not advertise general multilingual fluency, automatic translation, generated
lyrics, voice control or semantic search as shipped capabilities.

Windows x64 managed assets are available, but clean-Windows setup, actual model
execution and latency during a service still need evidence. Do not mark those
passed from Mac downloads or protocol-fixture tests. See the existing
[Luma evaluation](validation/luma-2026-11-25.md) for raw-model failures and guarded
app-level results; those are different measurements.

## NDI

NDI sending is optional at build time and requires the official SDK and appropriate
runtime/distribution rights. SDK-free builds expose capture diagnostics but do not
transmit a network source. Capture checks cover composition/alpha/queue behavior;
they do not verify network reception, receiver compatibility or audio transport.

Do not advertise network NDI support for an artifact whose manifest says
`ndiSenderCompiled: false`. SDK-enabled artifacts still need compilation against
the official headers and a real independent receiver rehearsal on each advertised
platform. Transparent overlays preserve authored media/text pixels, but network
alpha must be checked in a receiver before claiming interoperability.

## Distribution and updates

Windows offers a portable ZIP and optional per-user Inno Setup EXE. Signing hooks
are prepared; a real publisher identity and native signature evidence are still
required. macOS packaging remains ad-hoc signed until Developer ID/notarization
is configured. There is no claim of a signed public release yet.

The update checker downloads verified stable GitHub ZIP assets on Windows.
Website installers are run explicitly after quitting the app; automatic installer
execution, automatic install/restart and Store/MSIX updates are not implemented.
Keep the ZIP published alongside the installer until installer-aware updates are
implemented and tested. A checksum proves integrity, not publisher identity.
