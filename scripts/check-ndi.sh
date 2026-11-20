#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$project_dir/captures"
if ! test -f "$project_dir/captures/test-loop.mp4"; then
    command -v ffmpeg >/dev/null 2>&1 || { echo "Install FFmpeg to generate the video fixture." >&2; exit 1; }
    ffmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=640x360:rate=30' -t 8 -c:v libx264 -pix_fmt yuv420p -y "$project_dir/captures/test-loop.mp4"
fi
cmake --build "$project_dir/build"
ctest --test-dir "$project_dir/build" --output-on-failure
if [ "$(uname -s)" = Darwin ]; then
    app_binary="$project_dir/build/church-deck.app/Contents/MacOS/church-deck"
else
    app_binary="$project_dir/build/church-deck"
fi
exec "$app_binary" --fresh --ndi-smoke "$project_dir/captures"
