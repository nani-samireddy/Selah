#!/usr/bin/env python3
"""Record raw Needle routing; Selah adds deterministic references and Telugu wrappers."""
import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile
import time

CASES = [
    ("telugu-song", "songs", "కృప గురించి పాటలను వెతుకు"),
    ("mixed-song", "songs", "కృప songs చూపించు"),
    ("telugu-reference", "scripture", "యోహాను 3:16 చూపించు"),
    ("mixed-reference", "scripture", "show ఆదికాండము 1:1"),
    ("missing-book", "scripture", "show 3:16"),
    ("ambiguous", "scripture", "John or James 3:16"),
    ("english-reference", "scripture", "genesis 1:1"),
]
CONTEXT = (
    "Extract search terms or a Bible reference from the request. "
    "Tolerate spelling mistakes and correct obvious typos in search terms. "
    "Only propose the supplied search tools. Never generate scripture or "
    "change presentation output."
)


def schema(scope):
    song = scope == "songs"
    return [{
        "type": "function",
        "function": {
            "name": "search_songs" if song else "search_scripture",
            "description": (
                "Find saved songs by title, lyrics or tags. Does not add or present songs."
                if song else "Find a Bible passage by reference or words in the selected Bible."
            ),
            "parameters": {
                "type": "object",
                "properties": {"query": {
                    "type": "string", "minLength": 1, "maxLength": 256,
                    "description": (
                        "Words to search for in the local song library" if song
                        else "Bible reference such as John 3:16, or words to find"
                    ),
                }},
                "required": ["query"], "additionalProperties": False,
            },
        },
    }]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path)
    parser.add_argument("--output", type=Path, default=Path("luma-raw-evaluation.json"))
    args = parser.parse_args()
    windows = platform.system() == "Windows"
    root = args.directory
    if root is None:
        root = (Path(os.environ["APPDATA"]) / "ChurchDeck/Church Deck/ai/cactus-4.0.0"
                if windows else Path.home()
                / "Library/Application Support/ChurchDeck/Church Deck/ai/cactus-4.0.0")
    runtime = root / ("cactus.exe" if windows else "cactus")
    if not runtime.is_file() or not (root / "needle3.cact").is_file():
        parser.error("Download Luma first, or pass --directory with compatible native assets")
    rows = []
    with tempfile.TemporaryDirectory(prefix="selah-luma-evaluation-") as temporary:
        folder = Path(temporary)
        (folder / "context.txt").write_text(CONTEXT, encoding="utf-8")
        for name, scope, prompt in CASES:
            (folder / "tools.json").write_text(json.dumps(schema(scope)), encoding="utf-8")
            start = time.monotonic()
            result = subprocess.run([
                str(runtime), "--model", str(root / "needle3.cact"),
                "--tools", str(folder / "tools.json"), "--system", str(folder / "context.txt"),
                "--prompt", prompt,
            ], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=35,
                env={**os.environ, "CACTUS_TELEMETRY": "0", "HF_HUB_OFFLINE": "1"})
            try:
                response = json.loads(result.stdout)
            except json.JSONDecodeError:
                response = {"stdout": result.stdout[:1000], "stderr": result.stderr[:1000]}
            row = {
                "case": name, "scope": scope, "prompt": prompt,
                "seconds": round(time.monotonic() - start, 2),
                "exitCode": result.returncode, "response": response,
            }
            rows.append(row)
            print(json.dumps(row), flush=True)
    args.output.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
