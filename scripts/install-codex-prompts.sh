#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/prompts/codex"
DEST_DIR="${CODEX_HOME:-$HOME/.codex}/prompts"

if [[ ! -d "$SRC_DIR" ]]; then
  echo "Missing prompt templates at: $SRC_DIR" >&2
  exit 1
fi

shopt -s nullglob
files=("$SRC_DIR"/tdd-guard-*.md)
if [[ ${#files[@]} -eq 0 ]]; then
  echo "No prompt files found matching tdd-guard-*.md in $SRC_DIR" >&2
  exit 1
fi

mkdir -p "$DEST_DIR"
cp "${files[@]}" "$DEST_DIR"/

echo "Installed Codex prompts to: $DEST_DIR"
echo "Use /prompts:tdd-guard-on and /prompts:tdd-guard-off in Codex."
