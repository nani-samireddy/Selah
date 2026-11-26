# Preparing Selah for Windows

Prepared 26 November 2026. Windows x64 packaging supports a portable ZIP and an optional per-user EXE installer.
The existing updater continues to offer the portable ZIP. The website release target is the per-user installer, with the ZIP retained for
portable use and the existing updater. This prepares a release path; it does not approve
an artifact for publication. MSIX and automatic installation remain separate future work. Confirm minimum Windows version from deployed Qt/runtime
requirements and clean-machine tests before advertising support.

## Build on Windows

Use an x64 Visual Studio 2022 developer shell with Python 3.12+, CMake, Ninja and
Qt including Quick, Multimedia, ShaderTools, Sql, Network and required private headers.
The existing `.github/workflows/build-desktop.yml` provisions Qt 6.12.1 and MSVC
on Windows Server 2022; its uploaded artifacts are unsigned test candidates only.
The Windows job also compiles an unsigned testing installer with Inno Setup 6.
The workflow also builds Linux. No release publication or signing runs there.

```powershell
python scripts/package-desktop.py --jobs 4
```

Output: `dist/windows-x64/Selah-<version>-Windows-x64.zip`, SHA-256 sidecar,
release manifest and startup log. Deployment includes app-local MSVC runtime,
Qt plugins, SQLite and QML dependencies. The isolated offscreen startup check
is not a substitute for real display, GPU, audio or accessibility testing.

## Configure publisher signing

Obtain a publicly trusted code-signing identity through a provider appropriate for
your publisher and region. Complete identity verification and configure its token,
HSM or certificate-store integration on the signing Windows machine. Do not send
private keys or passwords in chat or commit them. Self-signed development certificates
are not the public-release solution. Costs and provider eligibility need confirmation
before purchase; no signing identity has been configured by these changes.

This implementation supports SignTool with a key exposed through the Windows `My`
certificate store (including compatible hardware-backed providers). A cloud service
requiring a custom SignTool plugin needs an adapter before use; it is not automatically
supported by setting these variables.

From a Windows SDK shell with `signtool.exe` on PATH:

```powershell
# These are public configuration values, not private keys.
$env:SELAH_SIGN_CERT_SHA1 = '<40 hexadecimal characters from your certificate>'
$env:SELAH_SIGN_TIMESTAMP_URL = '<your provider RFC3161 HTTP(S) timestamp URL>'
# Only for a certificate in Local Machine rather than Current User:
# $env:SELAH_SIGN_MACHINE_STORE = '1'
python scripts/package-desktop.py --jobs 4 --windows-sign
```

The signing flag validates configuration before compiling, selects the exact
certificate, signs `bin/Selah.exe` using SHA-256 plus an RFC3161 SHA-256 timestamp,
then runs Authenticode verification before smoke testing and archiving. Any signing
or verification failure aborts new package creation. Never substitute an older ZIP
left in the output directory after a failed run; use the successful run's manifest.

The manifest records certificate thumbprint and signature verification. Vendor DLLs
retain their original signatures; they are not re-signed as Selah. The ZIP itself is
not Authenticode signed; its checksum covers the final packaged bytes. Publishing
requires separate dependency/license review, including any unsigned vendor components.
A trusted signature does not guarantee a new app will avoid SmartScreen warnings.

## Release gates

- [ ] Set final version and Windows publisher/product metadata; record commit and toolchain.
- [ ] Choose/verify publisher signing identity and timestamp service; run signed packaging.
- [ ] Verify downloaded/extracted executable identity and signature on a clean standard-user PC.
- [ ] Start without Qt, Visual Studio or a developer PATH; test Unicode profile/install paths.
- [ ] Rehearse audio/video/standby media, physical outputs, mixed DPI, hot-plug and recovery.
- [ ] Complete Narrator, keyboard and contrast testing from the accessibility checklist.
- [ ] Transfer an offline program bundle from macOS; validate backup restore and data migration.
- [ ] Check licenses/notices and publish accurate optional Luma/NDI capabilities.
- [ ] Test a real stable update using the exact ZIP naming and manual replacement flow.
- [ ] Complete the RC evidence form, retain old version, then publish immutable assets/checksums.

Until these pass, label Windows packages as testing builds. The portable build uses extraction and manual updates. The optional installer
provides shortcuts and an uninstall entry; installation is still operator-initiated.

## References

- [Microsoft SignTool](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool)
- [Microsoft signing options](https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/code-signing-options)
- [Launch gates](launch-checklist-v1.md), [accessibility](accessibility-checklist.md),
  [RC evidence](release-candidate-record.md), [updates](release-updates.md)

## Generate the website installer

Install Inno Setup 6 on the Windows build machine. Set `SELAH_ISCC` to its
`ISCC.exe` if it is not on PATH or in the standard Program Files location.

```powershell
python scripts/package-desktop.py --jobs 4 --windows-installer --windows-publisher "Your legal name"
# Add --windows-sign after configuring the signing identity above.
```

This also creates `Selah-<version>-Windows-x64-Setup.exe`, its SHA-256 sidecar and
an installer entry in the external release manifest. The portable ZIP is retained.
Unsigned installers are testing candidates; website publication needs the signed build.

The installer uses a stable application ID, installs per user under
`%LOCALAPPDATA%\Programs\Selah`, adds a Start menu shortcut, offers an optional desktop
shortcut and registers an uninstaller. It includes the staged Qt/media/runtime files;
it does not download dependencies during installation or require admin rights by default.
Uninstall removes installed program files, not the separate ChurchDeck libraries/preferences.
No app launch is triggered automatically after installation.

With signing enabled, Inno's signing callback signs and verifies the generated
uninstaller, setup self-copies and installer, rather than only signing the final wrapper.
The application also exposes a Windows mutex checked by setup/uninstall: quit Selah
normally before continuing. Setup never force-closes a live service. Older builds
without the marker must be closed manually; test upgrades from those separately.

Before release, verify clean install, same-ID upgrade, cancelled install, installing
while Selah is running, uninstall while running and profile preservation after uninstall.
Test from a standard-user account and a browser-downloaded file. Native Windows
compilation and these install/uninstall rehearsals remain required; local Python
checks on macOS do not prove an installer works on Windows.

The 26 November CI candidate passed native model checks, Mac-bundle import, installer
compilation, active-app blocking, install/reinstall/startup/uninstall and profile
sentinel preservation. Its Windows-exported fixture also passed re-import on Mac.
See [the validation record](validation/launch-2026-11-26.md) for artifact hashes and
limits. These are unsigned testing artifacts, not publisher-signed public releases.
