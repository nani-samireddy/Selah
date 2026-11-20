#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
command -v ffmpeg >/dev/null 2>&1 || { echo "Install FFmpeg to generate media test fixtures." >&2; exit 1; }
mkdir -p "$project_dir/captures"
ffmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=640x360:rate=30' -t 8 -c:v libx264 -pix_fmt yuv420p -y "$project_dir/captures/test-loop.mp4"
ffmpeg -hide_banner -loglevel error -f lavfi -i 'color=c=0x486b52:size=640x360' -frames:v 1 -y "$project_dir/captures/test-card.png"
cmake --build "$project_dir/build"
ctest --test-dir "$project_dir/build" --output-on-failure
if [ "$(uname -s)" = Darwin ]; then
    app_binary="$project_dir/build/church-deck.app/Contents/MacOS/church-deck"
else
    app_binary="$project_dir/build/church-deck"
fi
exec "$app_binary" --fresh --media-smoke "$project_dir/captures/test-card.png" "$project_dir/captures/test-loop.mp4" "$project_dir/captures"
