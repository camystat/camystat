#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

say() { printf '%s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

say "Pulling submodules..."
git submodule update --init --recursive

say "Creating symlinks..."
mkdir -p "camystat/include"
(
  cd "camystat/include"
  ln -sfn "../../wxWidgets/include" "wxWidgets"
  ln -sfn "../../eigen" "eigen"
)

LICENSES_DIR="camystat/misc/licenses"
mkdir -p "$LICENSES_DIR"

install_deps_macos() {
  have brew || return 0

  say "Installing build dependencies via Homebrew..."
  brew install cmake pkg-config ccache curl || true
  # Use a stable Python with wide wheel availability.
  brew install python@3.12 || true
  brew install opencv wxwidgets || true
}

install_deps_linux() {
  if have apt-get; then
    say "Installing build dependencies via apt..."
    sudo apt-get update
    sudo apt-get install -y \
      build-essential cmake pkg-config curl ca-certificates \
      python3 python3-venv python3-pip \
      libopencv-dev libwxgtk3.2-dev
  fi
}

UNAME="$(uname -s)"
case "$UNAME" in
  Darwin) install_deps_macos ;;
  Linux) install_deps_linux ;;
  *) say "Skipping dependency install (unsupported OS: $UNAME)." ;;
esac

say "Preparing OpenCV headers..."
mkdir -p "camystat/include/opencv2"

opencv_include_dir=""
if have pkg-config && pkg-config --exists opencv4; then
  opencv_include_dir="$(pkg-config --cflags-only-I opencv4 | tr ' ' '\n' | sed -n 's/^-I//p' | head -n 1 || true)"
fi

if [[ -z "${opencv_include_dir}" ]]; then
  if [[ -d "/usr/include/opencv4" ]]; then
    opencv_include_dir="/usr/include/opencv4"
  elif [[ -d "/usr/local/include/opencv4" ]]; then
    opencv_include_dir="/usr/local/include/opencv4"
  elif [[ -d "/opt/homebrew/include/opencv4" ]]; then
    opencv_include_dir="/opt/homebrew/include/opencv4"
  fi
fi

if [[ -n "${opencv_include_dir}" && -d "${opencv_include_dir}/opencv2" ]]; then
  rm -rf "camystat/include/opencv2/opencv2"
  cp -R "${opencv_include_dir}/opencv2" "camystat/include/opencv2/opencv2"
  say "Copied OpenCV headers from ${opencv_include_dir}/opencv2"
else
  say "OpenCV headers not found. Install OpenCV (e.g. brew/apt) and re-run if needed."
fi

say "Preparing OpenCV libs (best effort)..."
mkdir -p "camystat/lib/opencv2"
opencv_lib_dir=""
if have pkg-config && pkg-config --exists opencv4; then
  opencv_lib_dir="$(pkg-config --variable=libdir opencv4 2>/dev/null || true)"
fi

if [[ -n "${opencv_lib_dir}" && -d "${opencv_lib_dir}" ]]; then
  shopt -s nullglob
  for f in "${opencv_lib_dir}"/libopencv*.so* "${opencv_lib_dir}"/libopencv*.dylib; do
    cp -f "$f" "camystat/lib/opencv2/" || true
  done
  shopt -u nullglob
fi

say "Collecting Eigen license..."
EIGEN_LICENSE_OUT="${LICENSES_DIR}/EIGEN_LICENSE.txt"
: > "$EIGEN_LICENSE_OUT"
while IFS= read -r -d '' f; do
  base="$(basename "$f")"
  if [[ "$base" == "COPYING.README" ]]; then
    continue
  fi
  {
    printf '\n====== %s ======\n\n' "$f"
    cat "$f"
    printf '\n=============\n'
  } >>"$EIGEN_LICENSE_OUT"
done < <(find "eigen" -maxdepth 1 -type f -name "COPYING.*" -print0 2>/dev/null || true)

say "Downloading wxWidgets license..."
if have curl; then
  curl -fsSL "https://raw.githubusercontent.com/wxWidgets/wxWidgets/master/docs/licence.txt" \
    -o "${LICENSES_DIR}/WXWIDGETS_LICENSE.txt"
else
  say "curl not available; skipping wxWidgets license download."
fi

say "Downloading OpenCV license..."
if have curl; then
  curl -fsSL "https://raw.githubusercontent.com/opencv/opencv/refs/heads/master/LICENSE" \
    -o "${LICENSES_DIR}/OPENCV_LICENSE.txt"
  curl -fsSL "https://raw.githubusercontent.com/opencv/opencv/refs/heads/master/3rdparty/ffmpeg/license.txt" \
    -o "${LICENSES_DIR}/OPENCV_LICENSE_FFMPEG.txt"
else
  say "curl not available; skipping OpenCV license download."
fi

say "Activating python venv..."
cd "plot"
PYTHON_BIN="${PYTHON_BIN:-}"
if [[ -z "$PYTHON_BIN" ]]; then
  if have python3.12; then
    PYTHON_BIN="python3.12"
  elif have python3.11; then
    PYTHON_BIN="python3.11"
  elif have python3.10; then
    PYTHON_BIN="python3.10"
  elif have python3; then
    PYTHON_BIN="python3"
  elif have python; then
    PYTHON_BIN="python"
  else
    die "Python not found (install python3.12 or python3)."
  fi
fi

"${PYTHON_BIN}" -c 'import sys; print(f"Using Python {sys.version.split()[0]}")'

rm -rf venv
"$PYTHON_BIN" -m venv venv
# shellcheck disable=SC1091
source "venv/bin/activate"

say "Installing build dependencies with pip..."
python -m pip install --upgrade pip setuptools wheel

# Matplotlib/Numpy often fail from source on fresh systems; prefer wheels.
if ! pip install --only-binary=:all: -r requirements.txt; then
  say "Binary wheels unavailable; falling back to source builds..."
  pip install -r requirements.txt
fi

say "Generating plot dependency licenses..."
pip install third-party-license-file-generator setuptools
if have third-party-license-file-generator; then
  third-party-license-file-generator -r requirements.txt -p "$(command -v python)"
elif python -c 'import importlib; importlib.import_module("third_party_license_file_generator")' >/dev/null 2>&1; then
  python -m third_party_license_file_generator -r requirements.txt -p "$(command -v python)"
else
  die "third-party-license-file-generator installed but not runnable in this Python environment."
fi

PLOT_LICENSE_OUT="../camystat/misc/licenses/PLOT_EXE_LICENSES.txt"
mkdir -p "../camystat/misc/licenses"
cp -f "THIRDPARTYLICENSES" "$PLOT_LICENSE_OUT"

if have curl; then
  for filename in \
    LICENSE \
    LICENSE_AMSFONTS \
    LICENSE_BAKOMA \
    LICENSE_CARLOGO \
    LICENSE_COLORBREWER \
    LICENSE_COURIERTEN \
    LICENSE_JSXTOOLS_RESIZE_OBSERVER \
    LICENSE_QT4_EDITOR \
    LICENSE_SOLARIZED \
    LICENSE_STIX \
    LICENSE_YORICK
  do
    tmp="$(mktemp)"
    if curl -fsSL "https://raw.githubusercontent.com/matplotlib/matplotlib/refs/heads/v3.10.x/LICENSE/${filename}" -o "$tmp"; then
      {
        printf '\n====== matplotlib/LICENSE/%s ======\n\n' "$filename"
        cat "$tmp"
        printf '\n=============\n'
      } >>"$PLOT_LICENSE_OUT"
    fi
    rm -f "$tmp"
  done
fi

say "Building plot.exe (via PyInstaller; filename kept as plot.exe for compatibility)..."
python -m pip install pyinstaller
export PYINSTALLER_CONFIG_DIR="${PWD}/.pyinstaller"
export PYINSTALLER_CACHE_DIR="${PWD}/.pyinstaller-cache"
mkdir -p "$PYINSTALLER_CONFIG_DIR" "$PYINSTALLER_CACHE_DIR"
export MPLCONFIGDIR="${PWD}/.matplotlib"
mkdir -p "$MPLCONFIGDIR"
pyinstaller --onefile plot.py

if [[ -f "dist/plot" ]]; then
  cp -f "dist/plot" "../camystat/plot.exe"
elif [[ -f "dist/plot.exe" ]]; then
  cp -f "dist/plot.exe" "../camystat/plot.exe"
else
  die "PyInstaller output not found in plot/dist/"
fi

cd "$ROOT_DIR"
say "Done"
