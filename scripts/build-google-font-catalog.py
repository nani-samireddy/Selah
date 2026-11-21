#!/usr/bin/env python3
"""Refresh the bundled full Google Fonts catalog from official, public metadata."""
from pathlib import Path
import json
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def read(url):
    request = urllib.request.Request(url, headers={"User-Agent": "ChurchDeck-catalog-builder"})
    with urllib.request.urlopen(request, timeout=90) as response:
        raw = response.read().decode()
    return json.loads(raw[raw.index("{"):])


def main():
    commit = read("https://api.github.com/repos/google/fonts/commits/main")["sha"]
    tree = read(f"https://api.github.com/repos/google/fonts/git/trees/{commit}?recursive=1")
    if tree.get("truncated"):
        raise RuntimeError("Google Fonts source tree was truncated; refusing an incomplete catalog")
    families = read("https://fonts.google.com/metadata/fonts")["familyMetadataList"]
    groups = {}
    for entry in tree["tree"]:
        parts = entry["path"].split("/")
        if entry["type"] == "blob" and len(parts) == 3 and parts[0] in ("ofl", "apache", "ufl"):
            groups.setdefault(parts[1], []).append(entry)
    records = []
    for family in families:
        key = re.sub("[^a-z0-9]", "", family["family"].lower())
        files = groups.get(key, [])
        fonts = [entry for entry in files if entry["path"].lower().endswith((".ttf", ".otf"))]
        licenses = [entry for entry in files if entry["path"].split("/")[-1].lower()
                    in ("ofl.txt", "license.txt", "licence.txt", "ufl.txt", "license")]
        weights = [weight for weight in family["fonts"] if not weight.endswith("i")]
        selected = min(weights or list(family["fonts"]), key=lambda weight: abs(int(weight.rstrip("i")) - 400))
        record = {"family": family["family"], "category": family["category"],
                  "subsets": [subset for subset in family["subsets"] if subset != "menu"],
                  "weight": int(selected.rstrip("i")), "italic": selected.endswith("i")}
        record["files"] = [{"path": entry["path"], "sha": entry["sha"],
                            "bytes": entry["size"], "font": entry in fonts}
                           for entry in licenses + fonts] if fonts and licenses else []
        if not record["files"]:
            record["css"] = True
        records.append(record)
    destination = ROOT / "assets/google-fonts/catalog.json"
    destination.write_text(json.dumps({"format": 1, "source": "https://fonts.google.com/metadata/fonts",
                                      "commit": commit, "families": records},
                                     ensure_ascii=False, separators=(",", ":")) + "\n")
    print(f"Bundled {len(records)} Google Fonts families, pinned source commit {commit}")


if __name__ == "__main__":
    main()
