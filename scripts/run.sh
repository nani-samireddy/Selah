#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if command -v brew >/dev/null 2>&1; then
    qt_prefix=$(brew --prefix)
    cmake -S "$project_dir" -B "$project_dir/build" -G Ninja -DCMAKE_PREFIX_PATH="$qt_prefix" -DQt6_DIR="$qt_prefix/lib/cmake/Qt6"
else
    cmake -S "$project_dir" -B "$project_dir/build"
fi
cmake --build "$project_dir/build"
if [ "$(uname -s)" = Darwin ]; then
    exec "$project_dir/build/church-deck.app/Contents/MacOS/church-deck" "$@"
else
    exec "$project_dir/build/church-deck" "$@"
fi
