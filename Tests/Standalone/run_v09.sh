#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp)"
trap 'rm -f "$OUT"' EXIT
g++ -std=c++14 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/Tests/Standalone" -I"$ROOT/Source/AshenVaults" \
  "$ROOT/Source/AshenVaults/AVEquipment.cpp" \
  "$ROOT/Source/AshenVaults/AVSkills.cpp" \
  "$ROOT/Tests/Standalone/EquipmentTests.cpp" -o "$OUT"
"$OUT"
