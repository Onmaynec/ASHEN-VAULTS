#!/usr/bin/env bash
# Optional Linux/WSL/macOS smoke test. Does not need UE4 or copyrighted files.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUTPUT="$(mktemp)"
trap 'rm -f "$OUTPUT"' EXIT
g++ -std=c++14 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/Tests/Standalone" -I"$ROOT/Source/AshenVaults" \
  "$ROOT/Source/AshenVaults/AVDungeonNavigation.cpp" \
  "$ROOT/Tests/Standalone/NavigationTests.cpp" \
  -o "$OUTPUT"
"$OUTPUT"
