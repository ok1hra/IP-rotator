#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

DO_PUBLISH=0
PUBLISH_ARGS=()

usage() {
  cat <<'EOF'
Usage: tools/all.sh [options] [-- publish-options...]

Run the full local recovery pipeline:
1. build SPIFFS image
2. build recovery web package
3. optionally publish to gh-pages

Options:
  --publish              Also publish build/recovery-web to gh-pages
  -h, --help             Show this help

Anything after `--` is passed to tools/publish_recovery_to_gh_pages.sh.

Examples:
  bash tools/all.sh
  bash tools/all.sh --publish
  bash tools/all.sh --publish -- --message "Publish REV 20260430"
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --publish)
      DO_PUBLISH=1
      shift
      ;;
    --)
      shift
      PUBLISH_ARGS=("$@")
      break
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

echo "==> Building SPIFFS image"
bash "${ROOT_DIR}/tools/build_spiffs_image.sh"

echo "==> Building recovery web package"
bash "${ROOT_DIR}/tools/build_recovery_package.sh"

if [[ "$DO_PUBLISH" -eq 1 ]]; then
  echo "==> Publishing recovery web to gh-pages"
  bash "${ROOT_DIR}/tools/publish_recovery_to_gh_pages.sh" "${PUBLISH_ARGS[@]}"
else
  echo "==> Build complete"
  echo "Recovery web is ready in: ${ROOT_DIR}/build/recovery-web"
  echo "To publish it, run:"
  echo "  bash tools/all.sh --publish"
fi
