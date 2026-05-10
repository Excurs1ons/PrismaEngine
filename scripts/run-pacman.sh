#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

CONFIG="debug"
ARCH_OVERRIDE=""

usage() {
    cat <<'EOF'
Usage: ./scripts/run-pacman.sh [options]

Build + run PacManGame with auto platform/arch detection.

Options:
  -c, --config <debug|release>   Build config (default: debug)
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

"${SCRIPT_DIR}/build.sh" --target pacman --config "${CONFIG}"

BIN="${REPO_ROOT}/build/${PRESET}/bin/PacManGame"
if [[ ! -x "${BIN}" ]]; then
    echo "PacMan executable not found: ${BIN}" >&2
    exit 1
fi

exec "${BIN}"
