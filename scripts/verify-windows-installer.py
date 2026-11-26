#!/usr/bin/env python3
"""Install/reinstall/uninstall a test candidate on a disposable Windows runner."""
import argparse
import ctypes
import importlib.util
import os
from pathlib import Path
import platform
import subprocess
import tempfile
import time


def run_installer(installer, directory, log):
    return subprocess.run([str(installer), "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART",
                           "/SP-", f"/DIR={directory}", f"/LOG={log}"], timeout=180).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("installer", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if platform.system() != "Windows":
        parser.error("Run only on a disposable Windows test runner")
    installer = args.installer.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location("package_desktop", Path(__file__).with_name("package-desktop.py"))
    package = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(package)
    # Never test against an existing operator installation. The stable app ID uses
    # a per-user uninstall key: require a fresh runner with no prior installation.
    import winreg
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                r"Software\Microsoft\Windows\CurrentVersion\Uninstall\{D749919D-FCEB-49D9-BA79-A922D60EDC54}_is1"):
            raise RuntimeError("An installed Selah already exists; use a fresh runner")
    except FileNotFoundError:
        pass
    with tempfile.TemporaryDirectory(prefix="selah install rehearsal ") as tmp:
        destination = Path(tmp)/"Selah"
        # A held app marker must block setup, including unattended operation.
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateMutexW.argtypes = [ctypes.c_void_p, ctypes.c_bool, ctypes.c_wchar_p]
        kernel.CreateMutexW.restype = ctypes.c_void_p
        kernel.CloseHandle.argtypes = [ctypes.c_void_p]
        marker = kernel.CreateMutexW(None, False, "Selah.Desktop.Running")
        if not marker:
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            code = run_installer(installer, destination, output/"blocked-install.log")
            if code == 0 or (destination/"bin/Selah.exe").exists():
                raise RuntimeError("Setup did not reject an active service marker")
        finally:
            kernel.CloseHandle(marker)
        # A synthetic library sentinel stays outside the installation tree.
        profile = Path(os.environ["APPDATA"])/"ChurchDeck"/"Church Deck"
        profile.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="installer-preservation-", dir=profile) as saved:
            sentinel = Path(saved)/"library-sentinel.txt"
            sentinel.write_text("Preserve operator content", encoding="utf-8")
            for name in ("install", "reinstall"):
                if run_installer(installer, destination, output/(name+".log")) != 0:
                    raise RuntimeError(f"{name} failed; see logs")
                if not (destination/"bin/Selah.exe").is_file():
                    raise RuntimeError("Installed executable missing")
                package.smoke(destination, output)
            uninstall = destination/"unins000.exe"
            subprocess.run([str(uninstall), "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART",
                            f"/LOG={output/'uninstall.log'}"], check=True, timeout=180)
            deadline = time.monotonic()+20
            while (destination/"bin/Selah.exe").exists() and time.monotonic() < deadline:
                time.sleep(0.2)
            if (destination/"bin/Selah.exe").exists() or sentinel.read_text(encoding="utf-8") != "Preserve operator content":
                raise RuntimeError("Uninstall failed or external profile sentinel was altered")
    (output/"installer-verification.txt").write_text(
        "PASS: active marker blocked install; per-user install/reinstall/startup/uninstall; external profile sentinel preserved.\n"
        "Not verified: prior-version migration, physical displays, hardware codecs, signing, screen readers or real libraries.\n")


if __name__ == "__main__":
    main()
