#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <source.png> <output.icns>" >&2
  exit 1
fi

icon_png="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
output_icns="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
iconset_dir="${output_icns%.icns}.iconset"

if [[ ! -f "$icon_png" ]]; then
  echo "error: icon png not found: $icon_png" >&2
  exit 1
fi

rm -rf "$iconset_dir"
mkdir -p "$iconset_dir"

make_icon() {
  local size="$1"
  local name="$2"
  sips -z "$size" "$size" "$icon_png" --out "$iconset_dir/$name" >/dev/null
}

make_icon 16 icon_16x16.png
make_icon 32 icon_16x16@2x.png
make_icon 32 icon_32x32.png
make_icon 64 icon_32x32@2x.png
make_icon 128 icon_128x128.png
make_icon 256 icon_128x128@2x.png
make_icon 256 icon_256x256.png
make_icon 512 icon_256x256@2x.png
make_icon 512 icon_512x512.png
make_icon 1024 icon_512x512@2x.png

iconutil -c icns "$iconset_dir" -o "$output_icns"
rm -rf "$iconset_dir"
