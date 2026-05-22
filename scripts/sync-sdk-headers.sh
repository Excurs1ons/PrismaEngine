#!/bin/bash
# sync-sdk-headers.sh
# Sync public engine headers from src/engine/ to sdk/include/PrismaEngine/
# This script is run during development to keep the SDK headers in sync.
# During CI/release, package-sdk.sh handles the full packaging flow.
#
# Usage: ./scripts/sync-sdk-headers.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_DIR="$PROJECT_ROOT/src/engine"
SDK_INCLUDE_DIR="$PROJECT_ROOT/sdk/include/PrismaEngine"

echo "Syncing SDK headers from src/engine/ → sdk/include/PrismaEngine/"
echo "  Source: $SRC_DIR"
echo "  Target: $SDK_INCLUDE_DIR"

# Create target directory
mkdir -p "$SDK_INCLUDE_DIR"

# Copy all .h files preserving directory structure
find "$SRC_DIR" -name "*.h" -type f | while read -r header; do
    relative_path="${header#$SRC_DIR/}"
    dest_dir="$SDK_INCLUDE_DIR/$(dirname "$relative_path")"
    mkdir -p "$dest_dir"
    cp "$header" "$dest_dir/"
    echo "  ${relative_path}"
done

echo ""
echo "SDK headers synced successfully."
echo "Total headers: $(find "$SDK_INCLUDE_DIR" -name "*.h" | wc -l)"
