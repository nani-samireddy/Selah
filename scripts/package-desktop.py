#!/usr/bin/env python3
"""Build and package Selah on Windows or Linux using Qt's native deployment API."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile
import time
from datetime import datetime, timezone

from build import ROOT, build_lock


def run(command, **kwargs):
    subprocess.run([str(item) for item in command], check=True, **kwargs)


def notices(stage):
    folder = stage / "Licenses"
    folder.mkdir()
    for source in (ROOT / "assets/fonts").rglob("*.txt"):
        if "OFL" in source.name:
            shutil.copy2(source, folder / (source.parent.name + "-" + source.name))
    for source in (ROOT / "third_party").rglob("*LICENSE*"):
        if source.is_file():
            shutil.copy2(source, folder / (source.parent.name + "-" + source.name))
    for source in (ROOT / "assets/scriptures").glob("*-notice.html"):
        shutil.copy2(source, folder / source.name)
    shutil.copy2(ROOT / "assets/LUCIDE-LICENSE", folder / "Lucide-LICENSE.txt")
    shutil.copy2(ROOT / "assets/template-packs/README.md", folder / "Template-assets.md")
    qr = (ROOT / "third_party/qrcodegen/qrcodegen.cpp").read_text()
    (folder / "qrcodegen-MIT.txt").write_text(qr[:qr.index("*/") + 2] + "\n")
    for name in ("LICENSE", "LICENSE.txt"):
        if (ROOT / name).is_file():
            shutil.copy2(ROOT / name, folder / ("Selah-" + name))
    if platform.system() == "Linux":
        for source in Path("/usr/share/doc").glob("*/copyright"):
            if source.parent.name.startswith(("libqt6", "qt6-", "libav", "libsw", "libssl")):
                shutil.copy2(source, folder / (source.parent.name + "-copyright"))


def compiler_runtime(stage):
    # Deploy app-local MSVC runtime DLLs rather than requiring a separate installer.
    base = os.environ.get("VCToolsRedistDir")
    roots = [Path(base)] if base else []
    vc = os.environ.get("VCINSTALLDIR")
    if vc:
        roots += sorted((Path(vc) / "Redist/MSVC").glob("*"), reverse=True)
    for root in roots:
        files = list(root.glob("x64/Microsoft.VC*.CRT/*.dll"))
        if files:
            for source in files:
                shutil.copy2(source, stage / "bin" / source.name)
            return
    if not list((stage / "bin").glob("vcruntime*.dll")):
        raise RuntimeError("MSVC runtime DLLs unavailable; run from an x64 VS developer shell")


def environment(stage):
    env = os.environ.copy()
    for key in ("QT_PLUGIN_PATH", "QML_IMPORT_PATH", "QML2_IMPORT_PATH", "QT_QML_IMPORT_PATH"):
        env.pop(key, None)
    if platform.system() == "Linux":
        env["LD_LIBRARY_PATH"] = str(stage / "lib")
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["QT_QUICK_BACKEND"] = "software"
    return env


def audit_linux(stage):
    issues, audited, seen = [], 0, set()
    for source in stage.rglob("*"):
        if not source.is_file() or source.resolve() in seen:
            continue
        seen.add(source.resolve())
        with source.open("rb") as handle:
            if handle.read(4) != b"\x7fELF":
                continue
        audited += 1
        result = subprocess.run(["ldd", str(source)], env=environment(stage), text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for line in result.stdout.splitlines():
            if "not found" in line or ("libQt6" in line and "=> /" in line and str(stage) not in line):
                issues.append({"binary": str(source.relative_to(stage)), "dependency": line.strip()})
    if issues:
        raise RuntimeError("Deployment audit failed:\n" + json.dumps(issues, indent=2))
    return {"elfBinariesAudited": audited, "missingDependencies": []}


def smoke(stage, output):
    exe = stage / "bin" / ("Selah.exe" if platform.system() == "Windows" else "Selah")
    if not exe.exists():
        raise RuntimeError("Installed executable missing: " + str(exe))
    log_path = output / "startup-validation.log"
    with log_path.open("w") as log, tempfile.TemporaryDirectory(prefix="selah-startup-") as cwd:
        process = subprocess.Popen([str(exe), "--fresh", "--workspace-smoke", cwd],
                                   cwd=cwd, env=environment(stage), stdout=log, stderr=subprocess.STDOUT)
        try:
            for _ in range(16):
                if process.poll() is not None:
                    raise RuntimeError("Packaged app exited at startup; see " + str(log_path))
                time.sleep(0.5)
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
    text = log_path.read_text(errors="replace")
    for marker in ("QQmlApplicationEngine failed", "Cannot load library",
                   "could not load the Qt platform plugin", "QSQLITE driver not loaded"):
        if marker.lower() in text.lower():
            raise RuntimeError("Packaged startup failed: " + marker)
    return {"startupPassed": True, "startupMode": "headless-software", "isolatedProfile": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args()
    system = platform.system()
    if system not in ("Linux", "Windows"):
        parser.error("Use package-macos.py on macOS")
    arch = {"AMD64": "x64", "x86_64": "x64", "aarch64": "arm64", "arm64": "arm64"}.get(platform.machine(), platform.machine())
    output = (args.output_dir or ROOT / "dist" / (system.lower() + "-" + arch)).resolve()
    output.mkdir(parents=True, exist_ok=True)
    build = ROOT / "build-release"
    with build_lock(build):
        with tempfile.TemporaryDirectory(prefix=".selah-package-", dir=output) as temp:
            stage = Path(temp) / "Selah"
            if not args.skip_build:
                run(["cmake", "--preset", "release", "-DCHURCHDECK_DEPLOY_RUNTIME=ON",
                     "-DCMAKE_INSTALL_PREFIX=" + str(stage)], cwd=ROOT)
                run(["cmake", "--build", build, "--parallel", max(1, args.jobs)])
            cache = (build / "CMakeCache.txt").read_text()
            for flag in ("CMAKE_BUILD_TYPE:STRING=Release", "BUILD_TESTING:BOOL=OFF",
                         "CHURCHDECK_ENABLE_DIAGNOSTICS:BOOL=OFF", "CHURCHDECK_DEPLOY_RUNTIME:BOOL=ON"):
                if flag not in cache:
                    raise RuntimeError("Invalid Release configuration: " + flag)
            run(["cmake", "--install", build, "--prefix", stage])
            if system == "Windows":
                compiler_runtime(stage)
                (stage / "Selah.cmd").write_text('@echo off\nstart "" "%~dp0bin\\Selah.exe" %*\n')
                audit = {"compilerRuntimeBundled": True}
            else:
                launcher = stage / "Selah"
                launcher.write_text('#!/bin/sh\nselah_bundle_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport LD_LIBRARY_PATH="$selah_bundle_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\nexec "$selah_bundle_dir/bin/Selah" "$@"\n')
                launcher.chmod(0o755)
                audit = audit_linux(stage)
            notices(stage)
            validation = smoke(stage, output)
            config = json.loads((build / "release-config.json").read_text())
            manifest = {"product": "Selah", **config, "platform": system, "architecture": arch,
                        "buildType": "Release", "developerDiagnostics": False, "signing": "unsigned",
                        "createdUTC": datetime.now(timezone.utc).isoformat(), **audit, **validation}
            if system == "Linux":
                manifest["buildDistribution"] = platform.freedesktop_os_release().get("PRETTY_NAME", "Linux")
                manifest["glibc"] = platform.libc_ver()[1]
            (stage / "release-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
            (stage / "README-release.txt").write_text(
                "Selah native Release\nRun Selah.cmd on Windows or ./Selah on Linux.\n"
                "Unsigned package. See release-manifest.json for the build platform and limits.\n"
                "Physical display and hardware media behavior need verification on your target system.\n"
                "NDI sending requires an SDK-enabled build; check ndiSenderCompiled.\n")
            name = f"Selah-{config['version']}-{system}-{arch}"
            if system == "Windows":
                archive = Path(shutil.make_archive(str(output / name), "zip", Path(temp), "Selah"))
            else:
                archive = output / (name + ".tar.gz")
                with tarfile.open(archive, "w:gz") as target:
                    target.add(stage, arcname="Selah")
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            (output / (archive.name + ".sha256")).write_text(digest + "  " + archive.name + "\n")
            manifest["artifact"] = {"file": archive.name, "bytes": archive.stat().st_size, "sha256": digest}
            (output / "release-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
            print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
