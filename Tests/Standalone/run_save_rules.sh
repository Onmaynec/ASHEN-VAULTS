#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp)"
trap 'rm -f "$OUT"' EXIT
g++ -std=c++14 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/Source/AshenVaults" \
  "$ROOT/Source/AshenVaults/AVSaveRules.cpp" \
  "$ROOT/Tests/Standalone/SaveRulesTests.cpp" -o "$OUT"
"$OUT"
