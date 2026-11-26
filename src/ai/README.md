# AI integration foundation

The optional Needle 3 assistant is available from the **Luma** toolbar button or
**Help → Luma**. It defaults to off until explicitly enabled. Proposals
are shown for review, and Search library performs the approved read-only searches.

## Boundaries

- `CactusBackend` starts a configured native executable with separate QProcess
  arguments, a local `.cact` file and temporary tool/context files. Inference
  does not run on the GUI/render thread. Requests have a 30-second deadline,
  cancellation and bounded stdout/stderr. No HTTP server or Python is required.
- `AiTools` defines two schemas and supplies only the selected Songs or Bible
  tool to the runtime. It validates the entire JSON proposal before
  accepting any action. Unknown tools, wrong argument types and low-confidence
  responses below 0.1 are rejected. Suggestions below 0.7 carry a review cue.
  Confidence is a routing hint, not an accuracy guarantee.
- `AiAssistant` depends only on `ContentLibrary`. A proposal must be reviewed and
  explicitly passed to `runSearches()`. Results are read from saved songs and
  installed Bible records. The assistant has no output, editor or program pointer.

The only tools are `search_songs(query)` and
`search_scripture(query)`. The operator selects the Bible in the UI; Selah attaches
its exact translation ID after decoding, so the model cannot choose or invent it.
Scripture queries reuse existing book
names/codes and reference parsing. Unknown translations fail the whole proposal.
Search groups contain at most 50 returned items; `total` is the count returned by
the catalog query, not an unrestricted database count.

Spelling suggestions use saved song titles, tags and lyrics, and installed Bible
book names. Exact query matches take priority. A unique nearby word can replace
a typo (including adjacent transpositions); short, unrelated and ambiguous words
stay unchanged. Corrected queries appear for approval before any search runs.
Bible book matching scales its edit allowance with both word lengths, including
missing letters, extra letters and adjacent swaps. It allows one edit in short
names, two in names of six or more letters, and three in names of ten or more,
with a one-third distance limit and no tied candidates.
Bible book names are also normalized before routing; chapter and verse numbers
are preserved. This does not rewrite saved content or generate Bible text.

## Setup contract

Call `configure()` with local file URLs for the platform's native Cactus runner
and a Needle `.cact` file, then explicitly enable the assistant. Installed paths
and the opt-in flag are preferences. Prompts, proposals and results are memory-only.
Only routing instructions and the selected tool schema accompany the prompt. Requests do not send
the library or live snapshot to a service. The child environment disables package
telemetry and requests offline Hugging Face behavior; the native runner and local
weights must be provisioned before use.

The adapter follows the documented CLI:

```
cactus --model <local-model> --tools <tools.json> --system <context.txt> --prompt <text>
```

Reference: [Cactus native deployment guide](https://www.cactuscompute.com/blog/needle-supported-devices).
A fresh process loads weights for each request in this preparation phase. A
persistent worker can be considered after latency and shutdown tests with real
runtime binaries. This foundation does not claim model accuracy or latency.

## Install and enable

Run `python3 scripts/install-ai.py` on macOS Apple Silicon or Windows x64. It
installs pinned native binaries and model weights in the app-data directory,
verifies SHA-256 hashes, and records revisions. It does not execute the runtime.
The AI dialog can enable the installed assets or use explicitly chosen local files.
The `--enable-ai` launch flag enables managed assets and opens the dialog.

## Release validation still required

1. Review the pinned runtime/model versions, checksums, licenses and supported architectures.
2. Validate the installer script and local-file setup on clean Windows and macOS installations.
3. Test actual Needle binaries on macOS Apple Silicon and Windows x64, including
   packaging, cancellation, repeated requests and output latency while presenting.
4. Evaluate English, Telugu and other target languages with church-specific
   commands, code-switching, names and ambiguous scripture references. Do not
   advertise unsupported language accuracy.
5. Keep presentation changes as explicit operator actions outside this tool set.

`tests/unit/AiAssistantTests.cpp` uses a native protocol fixture. It verifies
transport and validation without downloading a model; it is not an inference
benchmark. Voice input, semantic indexing, generated lyrics and scripture
translation are outside this initial scope.

`--ai-smoke` exercises the actual provisioned runtime through the AI dialog. On
macOS Apple Silicon, English song search and Genesis 1:1 lookup passed with exact
saved-library results and unchanged draft, program and live output. This narrow
check does not establish broader language accuracy or Windows compatibility.

## In-app managed setup

Open Luma → Setup → **Download Luma**. `ModelInstaller` downloads the pinned runtime,
Needle weights and license from Cactus Compute on Hugging Face. Each file streams
through `VerifiedDownload` into an atomic file and must pass its SHA-256. The UI
shows the current file, progress, cancellation and retryable errors. Verified files
are reused on retry. The total download is about 37 MB; it is separate from the app
package. Installation configures the paths but does not opt a new user into inference.
Use **Enable Luma** explicitly. Advanced local-file selection remains available.
Managed downloads support macOS ARM64 and Windows x64. Windows inference is not
certified by the Mac checks.

`AiRequest` handles explicit references using installed book codes, English names
and Telugu aliases before inference. It preserves chapter/verse/range numbers,
normalizes Unicode digits and asks for clarification for missing books, missing
chapters, multiple references, invalid ranges and unavailable verses. A small set
of Telugu/English song-search command wrappers is removed at the edges of the
request; the retained words are always from the user's text. These routes still
require review before searching. Free-form requests continue through the model;
this is not a claim of general Telugu model fluency.

Reproduce raw model routing with `python3 scripts/evaluate-luma.py --directory
<managed-assets> --output <report.json>`. The app-level `--ai-smoke` suite records
its route (model / request parser / clarification) and verifies exact saved content
and unchanged draft/program/live snapshots. Raw model results and guarded app
results must be reported separately.
