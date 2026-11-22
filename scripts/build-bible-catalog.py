#!/usr/bin/env python3
"""Refresh metadata for the app's Holy Bible XML repository picker, without downloading verses."""
import json
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = "nani-samireddy/Holy-Bible-XML-Format"
BRANCH = "master"


def main():
    # The contents API truncates directories at 1,000 entries; Git trees include the full catalog.
    url = f"https://api.github.com/repos/{REPOSITORY}/git/trees/{BRANCH}?recursive=1"
    request = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json"})
    with urllib.request.urlopen(request, timeout=30) as response:
        tree = json.load(response)
    if tree.get("truncated"):
        raise RuntimeError("GitHub returned a truncated Bible tree")
    records = []
    for item in tree["tree"]:
        filename = item["path"]
        if not filename.endswith(".xml") or "/" in filename:
            continue
        stem = filename[:-4]
        language = re.match(r"^[A-Z][a-z]+", stem)
        title = re.sub(r"(?<=[a-z])(?=[A-Z0-9])|(?<=[0-9])(?=[A-Za-z])", " ", stem)
        records.append(dict(title=title, filename=filename,
                            language=language.group() if language else "Other", bytes=item["size"]))
    preferred = ["Telugu", "English", "Hindi", "Tamil", "Kannada", "Malayalam", "Bengali", "Gujarati", "Marathi"]
    records.sort(key=lambda record: (record["language"] not in preferred,
                                    record["language"] != "Telugu", record["language"], record["title"]))
    output = ROOT / "assets/scriptures/bible-repository.json"
    output.write_text(json.dumps(dict(source=f"https://github.com/{REPOSITORY}", branch=BRANCH,
                                     commit=tree["sha"], bibles=records), indent=2) + "\n")
    print(f"Wrote metadata for {len(records)} Bible files to {output}")


if __name__ == "__main__":
    main()
