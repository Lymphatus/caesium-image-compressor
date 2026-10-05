#!/usr/bin/env bash
# Make build/ self-contained on Windows: Qt DLLs + plugins via windeployqt, plus caesium.dll and WinSparkle.dll.
#   source scripts/dev-env.sh && bash scripts/deploy-windows.sh [build-dir]
set -euo pipefail
BUILD="${1:-build}"
EXE="$BUILD/Caesium Image Compressor.exe"
[ -f "$EXE" ] || { echo "missing $EXE (build the app first)"; exit 1; }
cp -f "$BUILD/libcaesium-prefix/src/libcaesium/target/x86_64-pc-windows-gnu/release/caesium.dll" "$BUILD/"
cp -f "$BUILD/libwinsparkle-prefix/src/libwinsparkle/x64/Release/WinSparkle.dll" "$BUILD/"
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw "$EXE"
echo "deployed: $BUILD"
