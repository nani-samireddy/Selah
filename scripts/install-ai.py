#!/usr/bin/env python3
"""Install pinned, checksum-verified Needle assets for Selah's optional local AI."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import urllib.request

RUNTIME_REV = "1a028f5a3972b67dd8d8752f882113d475e7ec4c"
MODEL_REV = "c95d25b4efa1e9601e785d92438e4ade0dbddbf8"
RUNTIMES = {
    "macos-arm64": ("cactus", "39bd925a3759ad3dd18056918feffc82be43a92d4eb2ef5f8baf73a707ee3f3b"),
    "windows-x86_64": ("cactus.exe", "3727bc70f4ad4f5dcf3719b90ec01a3a5cbf7a79da9db8424b8ea64c7c4db1b2"),
}
MODEL_SHA = "c9d915eca282ed42d1a09b143b592adb4cc6744ffe2d294adf5cfc5548170c38"


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def download(url, path, expected=None):
    if expected and path.is_file() and digest(path) == expected:
        print(f"Verified {path.name}")
        return
    partial = path.with_suffix(path.suffix + ".part")
    try:
        with urllib.request.urlopen(url, timeout=60) as response, partial.open("wb") as output:
            while chunk := response.read(256 * 1024):
                output.write(chunk)
        if expected and digest(partial) != expected:
            raise RuntimeError(f"Checksum mismatch: {path.name}")
        partial.replace(path)
        print(f"Installed {path.name}")
    finally:
        partial.unlink(missing_ok=True)


def main():
    system, machine = platform.system(), platform.machine().lower()
    target = "macos-arm64" if system == "Darwin" and machine == "arm64" else (
        "windows-x86_64" if system == "Windows" and machine in ("amd64", "x86_64") else None)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path)
    parser.add_argument("--platform", choices=RUNTIMES, default=target)
    args = parser.parse_args()
    if not args.platform:
        parser.error("Choose a supported --platform and destination for this computer")
    if args.destination:
        destination = args.destination
    elif system == "Darwin":
        destination = Path.home() / "Library/Application Support/ChurchDeck/Church Deck/ai/cactus-4.0.0"
    elif system == "Windows":
        destination = Path(os.environ["APPDATA"]) / "ChurchDeck/Church Deck/ai/cactus-4.0.0"
    else:
        parser.error("Pass --destination when preparing assets on another OS")
    destination.mkdir(parents=True, exist_ok=True)
    name, sha = RUNTIMES[args.platform]
    download(f"https://huggingface.co/Cactus-Compute/cactus/resolve/{RUNTIME_REV}/releases/4.0.0/{args.platform}/{name}", destination / name, sha)
    download(f"https://huggingface.co/Cactus-Compute/needle3/resolve/{MODEL_REV}/needle3.cact", destination / "needle3.cact", MODEL_SHA)
    download(f"https://huggingface.co/Cactus-Compute/needle3/resolve/{MODEL_REV}/LICENSE", destination / "LICENSE-Needle.txt", "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30")
    if name == "cactus":
        (destination / name).chmod(0o755)
    receipt = {"runtimeRevision": RUNTIME_REV, "modelRevision": MODEL_REV,
               "platform": args.platform, "runtimeSha256": sha, "modelSha256": MODEL_SHA}
    (destination / "install-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(f"Ready in {destination}. Enable AI in Selah to use it.")


if __name__ == "__main__":
    main()
