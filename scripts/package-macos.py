#!/usr/bin/env python3
"""Build and package a self-contained, ad-hoc-signed macOS Release of Selah."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import plistlib
import re
import shutil
import stat
import subprocess
import tempfile

from build import ROOT, build_lock

MACHO_MAGIC = {b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xce\xfa\xed\xfe",
               b"\xfe\xed\xfa\xce", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"}


def run(command, **kwargs):
    subprocess.run([str(value) for value in command], check=True, **kwargs)


def audit(bundle):
    """Reject development-machine dependencies and record the actual OS floor."""
    binaries, seen, external, versions = [], set(), [], []
    for path in bundle.rglob("*"):
        if not path.is_file() or path.resolve() in seen:
            continue
        with path.open("rb") as handle:
            if handle.read(4) not in MACHO_MAGIC:
                continue
        seen.add(path.resolve())
        binaries.append(path)
        identities = set(subprocess.check_output(["otool", "-D", str(path)], text=True).splitlines()[1:])
        listing = subprocess.check_output(["otool", "-L", str(path)], text=True)
        for line in listing.splitlines()[1:]:
            dependency = line.strip().split(" (", 1)[0]
            if dependency in identities:
                continue
            if dependency.startswith("/") and not dependency.startswith(("/System/Library/", "/usr/lib/")):
                external.append({"binary": str(path.relative_to(bundle)), "dependency": dependency})
            if dependency.startswith("@rpath/"):
                candidate = bundle / "Contents/Frameworks" / dependency.removeprefix("@rpath/")
                if not candidate.exists():
                    external.append({"binary": str(path.relative_to(bundle)), "missingDependency": dependency})
            elif dependency.startswith("@loader_path/"):
                if not (path.parent / dependency.removeprefix("@loader_path/")).exists():
                    external.append({"binary": str(path.relative_to(bundle)), "missingDependency": dependency})
            elif dependency.startswith("@executable_path/"):
                if not (bundle / "Contents/MacOS" / dependency.removeprefix("@executable_path/")).exists():
                    external.append({"binary": str(path.relative_to(bundle)), "missingDependency": dependency})
        commands = subprocess.check_output(["otool", "-l", str(path)], text=True)
        versions += re.findall(r"\bminos\s+(\d+(?:\.\d+)+)", commands)
        versions += re.findall(r"LC_VERSION_MIN_MACOSX\s+cmdsize\s+\d+\s+version\s+(\d+(?:\.\d+)+)", commands)
    if external:
        raise RuntimeError("Unbundled dependencies:\n" + json.dumps(external, indent=2))
    minimum = max(versions, key=lambda value: tuple(map(int, value.split(".")))) if versions else "unknown"
    return {"machOBinariesAudited": len(binaries), "externalDependencies": external, "minimumMacOS": minimum}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-build", action="store_true", help="Package an existing runtime-only Release")
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    if platform.system() != "Darwin":
        parser.error("This packaging script runs on macOS.")
    build_dir = ROOT / "build-release"
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    version = re.search(r"project\(ChurchDeck VERSION ([\d.]+)", (ROOT / "CMakeLists.txt").read_text()).group(1)
    prefix = subprocess.check_output(["brew", "--prefix"], text=True).strip() if shutil.which("brew") else ""
    with build_lock(build_dir):
        if not args.skip_build:
            command = ["cmake", "--preset", "release"]
            if prefix:
                command += [f"-DCMAKE_PREFIX_PATH={prefix}", f"-DQt6_DIR={prefix}/lib/cmake/Qt6"]
            run(command, cwd=ROOT)
            run(["cmake", "--build", build_dir, "--parallel", max(1, args.jobs)])
        cache = (build_dir / "CMakeCache.txt").read_text()
        for required in ("CMAKE_BUILD_TYPE:STRING=Release", "BUILD_TESTING:BOOL=OFF", "CHURCHDECK_ENABLE_DIAGNOSTICS:BOOL=OFF"):
            if required not in cache:
                raise RuntimeError("Release configuration missing: " + required)
        source = build_dir / "Selah.app"
        if not (source / "Contents/MacOS/Selah").is_file():
            raise RuntimeError("Release executable not found. Build without --skip-build first.")
        with tempfile.TemporaryDirectory(prefix=".selah-package-", dir=output) as temp:
            stage = Path(temp)
            bundle = stage / "Selah.app"
            shutil.copytree(source, bundle, symlinks=True)
            # Discard development import paths; deployed Qt discovers its bundle-relative modules.
            (bundle / "Contents/Resources/qt.conf").unlink(missing_ok=True)
            seeded_frameworks = []
            if prefix:
                # These split Homebrew modules lack a discoverable cross-formula runpath.
                # Seed complete frameworks; the audit still checks their transitive dependencies.
                for name in ("QtSvg", "QtQuickTimeline", "QtQuickTimelineBlendTrees"):
                    framework = Path(prefix) / "lib" / (name + ".framework")
                    if framework.exists():
                        destination = bundle / "Contents/Frameworks" / framework.name
                        shutil.copytree(framework.resolve(), destination, symlinks=True)
                        binary = destination / "Versions/A" / name
                        listing = subprocess.check_output(["otool", "-L", str(binary)], text=True)
                        identity = subprocess.check_output(["otool", "-D", str(binary)], text=True).splitlines()[1].strip()
                        run(["install_name_tool", "-id", f"@rpath/{framework.name}/Versions/A/{name}", binary])
                        for line in listing.splitlines()[1:]:
                            dependency = line.strip().split(" (", 1)[0]
                            if dependency == identity or not dependency.startswith(prefix):
                                continue
                            relative = re.search(r"([^/]+\.framework/Versions/[^/]+/[^/]+)$", dependency)
                            if not relative:
                                raise RuntimeError("Unknown seeded-framework dependency: " + dependency)
                            run(["install_name_tool", "-change", dependency, "@rpath/" + relative.group(1), binary])
                        commands = subprocess.check_output(["otool", "-l", str(binary)], text=True)
                        if "path @loader_path/../../.. (" not in commands:
                            run(["install_name_tool", "-add_rpath", "@loader_path/../../..", binary])
                        seeded_frameworks.append(destination)
            notices = bundle / "Contents/Resources/Licenses"
            notices.mkdir(parents=True, exist_ok=True)
            for notice in (ROOT / "assets/fonts").glob("*-OFL.txt"):
                shutil.copy2(notice, notices / notice.name)
            for notice in (ROOT / "third_party").rglob("*LICENSE*"):
                if notice.is_file():
                    shutil.copy2(notice, notices / (notice.parent.name + "-" + notice.name))
            shutil.copy2(ROOT / "assets/LUCIDE-LICENSE", notices / "Lucide-LICENSE.txt")
            for notice in (ROOT / "assets/scriptures").glob("*-notice.html"):
                shutil.copy2(notice, notices / notice.name)
            qr_source = (ROOT / "third_party/qrcodegen/qrcodegen.cpp").read_text()
            (notices / "qrcodegen-MIT.txt").write_text(qr_source[:qr_source.index("*/") + 2] + "\n")
            if prefix:
                ffmpeg = Path(subprocess.check_output(["brew", "--prefix", "ffmpeg"], text=True).strip())
                for notice in [*ffmpeg.glob("COPYING*"), *ffmpeg.glob("LICENSE*")]:
                    if notice.is_file():
                        shutil.copy2(notice, notices / ("FFmpeg-" + notice.name))
            deploy = shutil.which("macdeployqt") or str(Path(prefix) / "bin/macdeployqt")
            command = [deploy, bundle, f"-qmldir={ROOT / 'qml'}", f"-qmlimport={build_dir}", "-codesign=-", "-verbose=2"]
            if prefix:
                # Homebrew splits Qt modules across formulae; this directory links all frameworks.
                command.append(f"-libpath={prefix}/lib")
            run(command)
            report = audit(bundle)
            for framework in seeded_frameworks:
                run(["codesign", "--force", "--sign", "-", framework])
            # Set the real dependency-derived OS floor before sealing the final app.
            info_path = bundle / "Contents/Info.plist"
            with info_path.open("rb") as handle:
                info = plistlib.load(handle)
            info["CFBundleShortVersionString"] = version
            info["CFBundleVersion"] = version
            if report["minimumMacOS"] != "unknown":
                info["LSMinimumSystemVersion"] = report["minimumMacOS"]
            info_path.chmod(info_path.stat().st_mode | stat.S_IWUSR)
            with info_path.open("wb") as handle:
                plistlib.dump(info, handle)
            run(["codesign", "--force", "--sign", "-", bundle])
            run(["codesign", "--verify", "--deep", "--strict", "--verbose=2", bundle])
            architectures = subprocess.check_output(["lipo", "-archs", str(bundle / "Contents/MacOS/Selah")], text=True).strip()
            label = f"Selah-{version}-macOS-{architectures.replace(' ', '-')}"
            zip_path = stage / (label + ".zip")
            run(["ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", bundle, zip_path])
            image_root = stage / "image"
            image_root.mkdir()
            # Link the already staged bundle into the image source without another full copy.
            shutil.move(str(bundle), image_root / "Selah.app")
            bundle = image_root / "Selah.app"
            (image_root / "Applications").symlink_to("/Applications")
            dmg_path = stage / (label + ".dmg")
            run(["hdiutil", "create", "-volname", "Selah", "-srcfolder", image_root, "-ov", "-format", "UDZO", dmg_path])
            run(["hdiutil", "verify", dmg_path])
            final_bundle = output / "Selah.app"
            if final_bundle.exists():
                shutil.rmtree(final_bundle)
            shutil.move(str(bundle), final_bundle)
            artifacts = []
            for artifact in (zip_path, dmg_path):
                final = output / artifact.name
                shutil.move(str(artifact), final)
                with final.open("rb") as handle:
                    digest = hashlib.file_digest(handle, "sha256").hexdigest()
                (output / (final.name + ".sha256")).write_text(digest + "  " + final.name + "\n")
                artifacts.append({"file": final.name, "bytes": final.stat().st_size, "sha256": digest})
            report.update({"product": "Selah", "version": version, "architecture": architectures,
                           "buildType": "Release", "developerDiagnostics": False, "testsIncluded": False,
                           "signing": "ad-hoc", "notarized": False,
                           "qtVersion": subprocess.check_output(["qmake", "-query", "QT_VERSION"], text=True).strip(),
                           "ndiSenderCompiled": "NDI_INCLUDE_DIR:PATH=NDI_INCLUDE_DIR-NOTFOUND" not in cache,
                           "createdUTC": datetime.now(timezone.utc).isoformat(), "artifacts": artifacts})
            (output / "release-manifest.json").write_text(json.dumps(report, indent=2) + "\n")
            (output / "README-release.txt").write_text(
                f"Selah {version}\nmacOS {report['minimumMacOS']} or newer; {architectures}.\n"
                "Release configuration, with developer tests/diagnostics disabled.\n"
                "Qt frameworks, QML modules and media dependencies are bundled.\n"
                "The app is ad-hoc signed for local testing, not Apple notarized.\n"
                + ("NDI sender compiled.\n" if report["ndiSenderCompiled"] else "Network NDI sending is unavailable because the official NDI SDK was not installed.\n")
                + "Saved libraries, programs and preferences use the existing application-data location.\n")
            print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
