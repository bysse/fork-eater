#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_BIN="${ROOT_DIR}/build/fork-eater"
DEST_DIR="${HOME}/bin"
DEST_BIN="${DEST_DIR}/fork-eater"

if [[ ! -x "${SRC_BIN}" ]]; then
    echo "Error: '${SRC_BIN}' does not exist or is not executable. Build first with ./build.sh." >&2
    exit 1
fi

mkdir -p "${DEST_DIR}"
install -m 755 "${SRC_BIN}" "${DEST_BIN}"
echo "Installed fork-eater to ${DEST_BIN}"
