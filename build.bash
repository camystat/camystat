#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "Built: build/camystat/Camystat.app"
  echo "Requires: brew install opencv wxwidgets (same machine that built or installed deps)"
  echo "Run:      open build/camystat/Camystat.app"
fi
