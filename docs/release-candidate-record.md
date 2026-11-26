# Selah release candidate evidence form

Copy this form per candidate into `docs/validation/` using its version and RC number.
Keep this template unfilled. Failed checks stay recorded; attach their retest evidence.

## Candidate identity

| Field | Value |
| --- | --- |
| Version / RC / commit SHA | Pending |
| Build date / CI run / toolchain / Qt version | Pending |
| Distribution channel and scope | Pending |
| macOS artifact / CPU / minimum supported OS / SHA-256 | Pending |
| Windows artifact / CPU / minimum supported OS / SHA-256 | Pending |
| Signing identities / notarization result / signature checks | Pending |
| Exact dependency/license inventory and notices | Pending |
| Release owner / accessibility reviewer / QA reviewer | Assign |

## Evidence ledger

For every row record result (pass/fail/not run/explicitly deferred), evidence link,
OS/hardware, date and reviewer. "Not applicable" needs a reason and scope approval.

| Gate | macOS | Windows | Evidence / issue / owner |
| --- | --- | --- | --- |
| Automated checks on candidate commit | Not run | Not run | Pending |
| Clean downloaded installation and signature/security checks | Not run | Not run | Pending |
| Upgrade, manual update and user-data preservation | Not run | Not run | Pending |
| VoiceOver / Narrator core workflows | Not run | Not run | Pending |
| Keyboard-only operation, focus, geometry and shortcuts | Not run | Not run | Pending |
| Contrast, scaling, reduced motion and transparency | Not run | Not run | Pending |
| English/Telugu, dual Scripture, shaping and splitting | Not run | Not run | Pending |
| Audio/video/GIF, standby looping and silent preview | Not run | Not run | Pending |
| Physical audience/stage routing, hot-plug and stop | Not run | Not run | Pending |
| Quick Add / Scripture during live service / resume | Not run | Not run | Pending |
| Crash recovery confirmation, outputs/audio remain off | Not run | Not run | Pending |
| Offline cross-platform program bundle transfer | Not run | Not run | Pending |
| Full backup/restore, corrupt file and low-disk handling | Not run | Not run | Pending |
| Custom cues: dry run, cancel, failure, network actions | Not run | Not run | Pending |
| Optional Luma setup/inference and NDI receiver checks | Not run | Not run | Scope decision pending |
| LAN remote pairing, permissions and accessibility | Not run | Not run | Pending |
| Full service rehearsal and duration/resource observations | Not run | Not run | Pending |

Use an isolated disposable profile for crash/restore/failure tests. Record display
layout, GPU, media fixtures and network conditions so a failure is reproducible.
Test shipped Release artifacts without developer SDKs or developer PATH dependencies.

## Decision

- Outstanding P0 issues: Pending audit.
- Deferred features and user-visible limits: Pending scope approval.
- Installation/help/privacy/notices/release-note URLs: Pending.
- Recovery/support contact and previous-version retention: Pending.
- Exact final artifact checksums reviewed after signing: Pending.
- macOS approval / reviewer / date: Not approved.
- Windows approval / reviewer / date: Not approved.
- Public download and real update-feed verification after publishing: Not run.

Approve only when every applicable release gate has evidence. Any changed binary
requires a new candidate/checksum and revalidation of affected gates.
