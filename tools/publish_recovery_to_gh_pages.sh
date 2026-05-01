#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PUBLISH_DIR="${ROOT_DIR}/build/recovery-web"
BRANCH="gh-pages"
REMOTE="origin"
COMMIT_MESSAGE=""
KEEP_TEMP=0

usage() {
  cat <<'EOF'
Usage: tools/publish_recovery_to_gh_pages.sh [options]

Publish the generated build/recovery-web/ directory to the gh-pages branch
without touching the current working tree.

Options:
  --publish-dir PATH     Directory to publish (default: ./build/recovery-web)
  --branch NAME          Target Pages branch (default: gh-pages)
  --remote NAME          Git remote to push to (default: origin)
  --message TEXT         Commit message for the publish commit
  --keep-temp            Keep the temporary publish workspace
  -h, --help             Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --publish-dir)
      PUBLISH_DIR="$2"
      shift 2
      ;;
    --branch)
      BRANCH="$2"
      shift 2
      ;;
    --remote)
      REMOTE="$2"
      shift 2
      ;;
    --message)
      COMMIT_MESSAGE="$2"
      shift 2
      ;;
    --keep-temp)
      KEEP_TEMP=1
      shift
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

if [[ ! -d "$PUBLISH_DIR" ]]; then
  echo "Publish directory not found: $PUBLISH_DIR" >&2
  echo "Build it first with:" >&2
  echo "  bash tools/build_spiffs_image.sh" >&2
  echo "  bash tools/build_recovery_package.sh" >&2
  exit 1
fi

if ! git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "Not a git repository: $ROOT_DIR" >&2
  exit 1
fi

if ! git -C "$ROOT_DIR" remote get-url "$REMOTE" >/dev/null 2>&1; then
  echo "Git remote not found: $REMOTE" >&2
  exit 1
fi

if [[ -z "$COMMIT_MESSAGE" ]]; then
  COMMIT_MESSAGE="Publish recovery web $(date -u +%Y-%m-%dT%H:%M:%SZ)"
fi

TMP_DIR="$(mktemp -d)"
cleanup() {
  if [[ "$KEEP_TEMP" -eq 0 ]]; then
    rm -rf "$TMP_DIR"
  else
    echo "Temporary publish workspace kept at: $TMP_DIR"
  fi
}
trap cleanup EXIT

git init "$TMP_DIR" >/dev/null
git -C "$TMP_DIR" remote add "$REMOTE" "$(git -C "$ROOT_DIR" remote get-url "$REMOTE")"

if git -C "$TMP_DIR" ls-remote --exit-code --heads "$REMOTE" "$BRANCH" >/dev/null 2>&1; then
  git -C "$TMP_DIR" fetch --depth 1 "$REMOTE" "$BRANCH"
  git -C "$TMP_DIR" checkout -B "$BRANCH" "FETCH_HEAD"
else
  git -C "$TMP_DIR" checkout --orphan "$BRANCH"
fi

find "$TMP_DIR" -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} +
cp -R "${PUBLISH_DIR}/." "$TMP_DIR/"
touch "$TMP_DIR/.nojekyll"

git -C "$TMP_DIR" add --all

if git -C "$TMP_DIR" diff --cached --quiet; then
  echo "No publish changes detected for branch $BRANCH."
  exit 0
fi

if [[ -z "$(git -C "$TMP_DIR" config user.name || true)" ]]; then
  git -C "$TMP_DIR" config user.name "$(git -C "$ROOT_DIR" config user.name || echo IP-rotator Publisher)"
fi
if [[ -z "$(git -C "$TMP_DIR" config user.email || true)" ]]; then
  git -C "$TMP_DIR" config user.email "$(git -C "$ROOT_DIR" config user.email || echo publish@example.invalid)"
fi

git -C "$TMP_DIR" commit -m "$COMMIT_MESSAGE"

echo "Pushing ${PUBLISH_DIR} to ${REMOTE}/${BRANCH}..."
git -C "$TMP_DIR" push "$REMOTE" "$BRANCH"

echo "Publish complete."
echo "Configure GitHub Pages to serve branch '$BRANCH' from '/ (root)'."
