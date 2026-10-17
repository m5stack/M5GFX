#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
out=$(mktemp -d "${TMPDIR:-/tmp}/m5gfx-board-detect.XXXXXX")
trap 'rm -rf "$out"' EXIT HUP INT TERM
if [ -n "${CI:-}" ] && [ "${M5GFX_UPDATE_SNAPSHOTS+x}" = x ]; then
  echo "Snapshot updates are forbidden in CI" >&2
  exit 1
fi
count=0
failed=0
for source in "$root"/test/board_detect/*.cpp; do
  name=$(basename "$source" .cpp)
  "${CXX:-c++}" -std=c++11 -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -pedantic \
    '-DM5GFX_BOARD_DETECT_HOST_HAL="lgfx/v1/platforms/esp32/common.hpp"' \
    '-DM5GFX_BOARD_DETECT_HOST_TOUCH="board_detect/m5/atom_touch.inl"' \
    -I"$root/test/board_detect/stubs" -I"$root/src" \
    "$source" -o "$out/$name"
  if (cd "$root"
      "$out/$name"); then
    echo "$name: passed"
    count=$((count+1))
  else
    echo "$name: failed" >&2
    failed=$((failed+1))
  fi
done
echo "C++ test programs: $count passed"
if [ "$failed" -ne 0 ]; then
  echo "C++ test programs: $failed failed" >&2
  exit 1
fi
