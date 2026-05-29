#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_DIR="$ENGINE_ROOT/build/android-arm64-debug"
JNILIBS_DIR="$SCRIPT_DIR/app/jniLibs/arm64-v8a"

echo "=== PrismaAndroid APK Builder ==="

echo "=== Step 1: Building Engine ==="
cmake --preset android-arm64-debug \
    -DPRISMA_BUILD_PROJECT_PRISMA2D=OFF \
    -DPRISMA_BUILD_PROJECT_PATHTRACING3D=OFF \
    -DPRISMA_BUILD_PROJECT_PRISMACRAFT=OFF \
    -DPRISMA_BUILD_PROJECT_PACMAN=OFF \
    -DPRISMA_BUILD_PROJECT_NEOEDITOR=OFF
cmake --build "$BUILD_DIR" --parallel

echo "=== Step 2: Copying .so to jniLibs ==="
mkdir -p "$JNILIBS_DIR"

if [ -f "$BUILD_DIR/lib/libPrisma.so" ]; then
    cp "$BUILD_DIR/lib/libPrisma.so" "$JNILIBS_DIR/"
fi

SDL3_SO=$(find "$BUILD_DIR/_deps/sdl3-build" -name "libSDL3.so" 2>/dev/null | head -1)
if [ -n "$SDL3_SO" ]; then
    cp "$SDL3_SO" "$JNILIBS_DIR/"
fi

CS_RUNTIME_DIR="$BUILD_DIR/PrismaEngine.Host/android-arm64/publish"
if [ -d "$CS_RUNTIME_DIR" ]; then
    for so_file in "$CS_RUNTIME_DIR"/lib*.so; do
        if [ -f "$so_file" ]; then
            cp "$so_file" "$JNILIBS_DIR/"
        fi
    done
fi

echo "=== Step 3: Building APK ==="
cd "$SCRIPT_DIR"
./gradlew assembleDebug

echo "=== Done ==="
echo "APK: $SCRIPT_DIR/app/build/outputs/apk/debug/app-debug.apk"
