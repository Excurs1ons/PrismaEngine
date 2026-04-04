#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

CONFIG="release"
ARCH_OVERRIDE=""

usage() {
    cat <<'EOF'
Usage: ./scripts/package-pacman.sh [options]

Build PacManGame and create a distributable tar.gz package.

Options:
  -c, --config <debug|release>   Build config (default: release)
  -a, --arch <x64|arm64>         Override detected arch
  -h, --help                     Show help
EOF
}

normalize_arch() {
    case "$(uname -m)" in
        x86_64|amd64) echo "x64" ;;
        aarch64|arm64) echo "arm64" ;;
        *) echo "unknown" ;;
    esac
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--config) CONFIG="${2:-}"; shift 2 ;;
        -a|--arch) ARCH_OVERRIDE="${2:-}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage; exit 1 ;;
    esac
done

case "${CONFIG}" in
    debug|release) ;;
    *) echo "Invalid --config: ${CONFIG}" >&2; exit 1 ;;
esac

ARCH="${ARCH_OVERRIDE:-$(normalize_arch)}"
PRESET="pacman-linux-${ARCH}-${CONFIG}"
OUT_ROOT="${REPO_ROOT}/dist"
PKG_DIR="${OUT_ROOT}/PacManGame-linux-${ARCH}-${CONFIG}"
PKG_FILE="${PKG_DIR}.tar.gz"

"${SCRIPT_DIR}/build.sh" --target pacman --config "${CONFIG}"

BIN_DIR="${REPO_ROOT}/build/${PRESET}/bin"
BIN_PATH="${BIN_DIR}/PacManGame"
if [[ ! -x "${BIN_PATH}" ]]; then
    echo "PacMan executable not found: ${BIN_PATH}" >&2
    exit 1
fi

rm -rf "${PKG_DIR}"
mkdir -p "${PKG_DIR}"
cp -a "${BIN_PATH}" "${PKG_DIR}/"
if [[ -d "${BIN_DIR}/assets" ]]; then
    cp -a "${BIN_DIR}/assets" "${PKG_DIR}/"
fi

cat > "${PKG_DIR}/README.txt" <<EOF
PacManGame package (${CONFIG}, ${ARCH})

Run:
  ./PacManGame

If no graphics device is available, the game will switch to console view.
EOF

mkdir -p "${OUT_ROOT}"
tar -czf "${PKG_FILE}" -C "${OUT_ROOT}" "$(basename "${PKG_DIR}")"

echo "Package created: ${PKG_FILE}"
