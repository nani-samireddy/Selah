"""Build the Windows x64 per-user Inno Setup installer from a deployed stage."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from build import ROOT
import windows_signing


def compiler():
    explicit = os.environ.get("SELAH_ISCC")
    if explicit:
        path = Path(explicit).resolve(strict=True)
        if not path.is_file():
            raise RuntimeError("SELAH_ISCC must point to ISCC.exe")
        return str(path)
    tool = shutil.which("ISCC.exe") or shutil.which("iscc")
    if tool:
        return tool
    for variable in ("ProgramFiles(x86)", "ProgramFiles"):
        root = os.environ.get(variable)
        if root:
            path = Path(root) / "Inno Setup 6/ISCC.exe"
            if path.is_file():
                return str(path)
    raise RuntimeError("Install Inno Setup 6 and set SELAH_ISCC to ISCC.exe, or add it to PATH")


def validate_metadata(version, publisher):
    if not re.fullmatch(r"\d+\.\d+\.\d+", version) or any(int(n) > 65535 for n in version.split(".")):
        raise RuntimeError("Installer requires a numeric major.minor.patch version (each <=65535)")
    if not publisher.strip() or any(c in publisher for c in '\r\n"{}'):
        raise RuntimeError("--windows-publisher must be a nonempty publisher name without quotes/braces/newlines")


def build(stage, output, version, publisher, iscc, signing_config=None):
    validate_metadata(version, publisher)
    stage, output = Path(stage).resolve(), Path(output).resolve()
    if not (stage / "bin/Selah.exe").is_file():
        raise RuntimeError("Installer stage is missing bin/Selah.exe")
    output.mkdir(parents=True, exist_ok=True)
    name = f"Selah-{version}-Windows-x64-Setup"
    command = [iscc, "/Qp", f"/DAppVersion={version}", f"/DPublisher={publisher}",
               f"/DStageDir={stage}", f"/DAppIcon={ROOT / 'assets/branding/church-deck.ico'}",
               f"/O{output}", f"/F{name}"]
    if signing_config:
        # Inno signs the uninstaller and setup self-copies too. The Python callback
        # uses the same validated environment and verifies every resulting signature.
        callback = subprocess.list2cmdline([sys.executable, str(ROOT / "scripts/windows_signing.py")])
        callback = callback.replace("$", "$$") + " $f"
        command += ["/DSignedRelease=1", "/SSelahRelease=" + callback]
    command.append(str(ROOT / "packaging/windows/selah.iss"))
    subprocess.run(command, check=True)
    installer = output / (name + ".exe")
    if not installer.is_file() or installer.stat().st_size == 0:
        raise RuntimeError("Inno Setup did not produce the expected installer")
    if signing_config:
        windows_signing.verify_executable(installer, signing_config)
    return installer
