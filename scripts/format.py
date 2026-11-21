#!/usr/bin/env python3
"""Apply the repository's C++ and QML formatters, or verify them with --check."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def find_tool(name: str, alternatives: tuple[str, ...] = ()) -> str:
    candidate = shutil.which(name)
    if candidate:
        return candidate
    for path in alternatives:
        if Path(path).is_file():
            return path
    raise RuntimeError(f"{name} is required; see docs/coding-standards.md")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="report differences without editing")
    args = parser.parse_args()
    clang = find_tool("clang-format", (
        "/Library/Developer/CommandLineTools/usr/bin/clang-format",
        "/opt/homebrew/opt/llvm/bin/clang-format",
    ))
    qml = find_tool("qmlformat")
    cpp_files = sorted(path for folder in ("src", "tests") for path in (ROOT / folder).rglob("*")
                       if path.suffix in (".cpp", ".h"))
    failed = False
    if args.check:
        result = subprocess.run([clang, "--dry-run", "--Werror", *map(str, cpp_files)],
                                cwd=ROOT, check=False)
        failed = result.returncode != 0
    else:
        # Preserve unchanged timestamps so formatting does not force a full native rebuild.
        for path in cpp_files:
            result = subprocess.run([clang, str(path)], cwd=ROOT, capture_output=True, check=False)
            if result.returncode:
                sys.stderr.buffer.write(result.stderr)
                failed = True
            elif result.stdout != path.read_bytes():
                path.write_bytes(result.stdout)
    for path in sorted((ROOT / "qml").rglob("*.qml")):
        # Comparing stdout avoids Qt-version-specific flags for formatting verification.
        result = subprocess.run([qml, str(path)], cwd=ROOT, capture_output=True, check=False)
        if result.returncode:
            print(f"qmlformat failed: {path.relative_to(ROOT)} (exit {result.returncode})", file=sys.stderr)
            sys.stderr.buffer.write(result.stderr)
            failed = True
            continue
        if args.check:
            if result.stdout != path.read_bytes():
                print(f"Formatting differs: {path.relative_to(ROOT)}", file=sys.stderr)
                failed = True
        elif result.stdout != path.read_bytes():
            path.write_bytes(result.stdout)
    if not failed:
        print("C++ and QML formatting checks passed." if args.check else "C++ and QML formatted.")
    return int(failed)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        print(error, file=sys.stderr)
        sys.exit(2)
