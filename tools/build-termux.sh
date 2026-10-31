#!/data/data/com.termux/files/usr/bin/bash
# Build an Android APK on-device with conservative memory usage.
set -euo pipefail

cd "$(dirname "$0")/.."

if ! command -v termux-setup-storage >/dev/null 2>&1; then
    echo "This helper is intended for Termux on Android." >&2
    exit 1
fi

for tool in make python3 clang wget unzip zip apksigner; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing $tool. Install dependencies: pkg install git wget unzip make python zip apksigner clang libglvnd libglvnd-dev" >&2
        exit 1
    fi
done

if [ ! -f "${PREFIX}/include/GLES2/gl2platform.h" ]; then
    echo "Missing GLES2/gl2platform.h: pkg install libglvnd libglvnd-dev" >&2
    exit 1
fi

version="${VERSION:-us}"
if [ ! -f "baserom.${version}.z64" ]; then
    echo "Missing baserom.${version}.z64 in repository root." >&2
    exit 1
fi

if [ ! -f SDL/include/SDL.h ]; then
    bash getSDL.sh
fi

# Compile one file at a time: Android may kill multiple concurrent clang jobs
# when memory becomes constrained. Existing objects are preserved.
make -j1 "VERSION=${version}" "$@"

apk="build/${version}_pc/sm64.${version}.f3dex2e.apk"
if [ ! -f "$apk" ]; then
    echo "Expected APK not found at $apk." >&2
    exit 1
fi
apksigner verify --verbose "$apk"
printf 'Signed APK: %s\n' "$apk"
