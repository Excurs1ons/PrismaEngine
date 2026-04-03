#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

TARGET="engine"
CONFIG="debug"
PRESET_OVERRIDE=""
CLEAN=0
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

usage() {
    cat <<'EOF'
Usage: ./scripts/build.sh [options]

One-click build with auto platform/arch preset selection.

Options:
  -t, --target <engine|editor|runtime>  Build target (default: engine)
  -c, --config <debug|release>          Build config (default: debug)
  -p, --preset <name>                   Explicit preset (skip auto detect)
      --clean                           Remove selected build directory first
  -j, --jobs <N>                        Parallel jobs (default: nproc)
  -h, --help                            Show help

Examples:
  ./scripts/build.sh
  ./scripts/build.sh --target editor --config release
  ./scripts/build.sh --preset engine-linux-arm64-debug --clean
EOF
}

normalize_arch() {
    case "$(uname -m)" in
        x86_64|amd64) echo "x64" ;;
        aarch64|arm64) echo "arm64" ;;
        armv7l|armv7) echo "armv7" ;;
        *) echo "unknown" ;;
    esac
}

normalize_platform() {
    case "$(uname -s)" in
        Linux) echo "linux" ;;
        Darwin) echo "macos" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "unknown" ;;
    esac
}

preset_exists() {
    local preset="$1"
    cmake --list-presets 2>/dev/null | sed -n 's/.*"\([^"]*\)".*/\1/p' | grep -Fxq "${preset}"
}

build_dir_from_preset() {
    local preset="$1"
    sed -n '/"configurePresets"[[:space:]]*:/,/"buildPresets"[[:space:]]*:/p' CMakePresets.json \
        | awk -v p="$preset" '
            $0 ~ /"name"[[:space:]]*:/ && $0 ~ "\"" p "\"" { found=1 }
            found && $0 ~ /"binaryDir"[[:space:]]*:/ {
                gsub(/.*"binaryDir"[[:space:]]*:[[:space:]]*"/, "", $0)
                gsub(/".*/, "", $0)
                gsub(/\$\{sourceDir\}/, ".", $0)
                print $0
                exit
            }
        '
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--target)
            TARGET="${2:-}"; shift 2 ;;
        -c|--config)
            CONFIG="${2:-}"; shift 2 ;;
        -p|--preset)
            PRESET_OVERRIDE="${2:-}"; shift 2 ;;
        --clean)
            CLEAN=1; shift ;;
        -j|--jobs)
            JOBS="${2:-}"; shift 2 ;;
        -h|--help)
            usage; exit 0 ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            exit 1 ;;
    esac
done

case "${TARGET}" in
    engine|editor|runtime) ;;
    *) echo "Invalid --target: ${TARGET}" >&2; exit 1 ;;
esac

case "${CONFIG}" in
    debug|release) ;;
    *) echo "Invalid --config: ${CONFIG}" >&2; exit 1 ;;
esac

PLATFORM="$(normalize_platform)"
ARCH="$(normalize_arch)"

if [[ -n "${PRESET_OVERRIDE}" ]]; then
    PRESET="${PRESET_OVERRIDE}"
else
    PRESET="${TARGET}-${PLATFORM}-${ARCH}-${CONFIG}"
fi

if ! preset_exists "${PRESET}"; then
    echo "Auto-selected preset not found: ${PRESET}" >&2
    echo "Detected platform=${PLATFORM}, arch=${ARCH}, target=${TARGET}, config=${CONFIG}" >&2
    echo "Available configure presets:" >&2
    cmake --list-presets >&2
    exit 1
fi

BUILD_DIR="$(build_dir_from_preset "${PRESET}")"
if [[ -z "${BUILD_DIR}" ]]; then
    BUILD_DIR="build/${PRESET}"
fi

echo "Detected platform : ${PLATFORM}"
echo "Detected arch     : ${ARCH}"
echo "Selected target   : ${TARGET}"
echo "Selected config   : ${CONFIG}"
echo "Selected preset   : ${PRESET}"
echo "Output directory  : ${BUILD_DIR}"

if [[ ${CLEAN} -eq 1 ]]; then
    echo "Cleaning ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

cmake --preset "${PRESET}"
cmake --build --preset "${PRESET}" -j"${JOBS}"

echo "Build completed: ${PRESET}"
