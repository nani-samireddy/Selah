#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$project_dir/captures"
if ! test -f "$project_dir/captures/test-loop.mp4"; then
    command -v ffmpeg >/dev/null 2>&1 || { echo "Install FFmpeg to generate the video fixture." >&2; exit 1; }
    ffmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=640x360:rate=30' -t 8 -c:v libx264 -pix_fmt yuv420p -y "$project_dir/captures/test-loop.mp4"
fi
python3 "$project_dir/scripts/build.py"
ctest --test-dir "$project_dir/build" --output-on-failure
if [ "$(uname -s)" = Darwin ]; then
    app_binary="$project_dir/build/Selah.app/Contents/MacOS/Selah"
else
    app_binary="$project_dir/build/Selah"
fi
exec "$app_binary" --fresh --ndi-smoke "$project_dir/captures"
