#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
python3 "$project_dir/scripts/build.py" --configure
if [ "$(uname -s)" = Darwin ]; then
    exec "$project_dir/build/Selah.app/Contents/MacOS/Selah" "$@"
else
    exec "$project_dir/build/Selah" "$@"
fi
