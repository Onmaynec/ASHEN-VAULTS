#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp)"
trap 'rm -f "$OUT"' EXIT
g++ -std=c++14 -O2 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/Tests/Standalone" -I"$ROOT/Source/AshenVaults" \
  "$ROOT/Source/AshenVaults/AVContent.cpp" \
  "$ROOT/Source/AshenVaults/AVDungeonNavigation.cpp" \
  "$ROOT/Source/AshenVaults/AVDepthDirector.cpp" \
  "$ROOT/Tests/Standalone/DepthDirectorTests.cpp" -o "$OUT"
"$OUT"
