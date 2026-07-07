#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CHECK_TREE=(Makefile README.md current_task.md docs scripts)

echo "[hz11-standalone] checking required docs"
for file in \
  README.md \
  current_task.md \
  docs/README.md \
  docs/HZ11_POSITIONING_L0.md \
  docs/HZ11_THREAD_CACHE_FAST_PATH_L0.md \
  docs/HZ11_NO_GO_LEDGER.md; do
  if [[ ! -f "$file" ]]; then
    echo "[hz11-standalone] missing required file: ${file}" >&2
    exit 1
  fi
done

echo "[hz11-standalone] checking symlinks"
if find "${CHECK_TREE[@]}" -type l | grep -q .; then
  find "${CHECK_TREE[@]}" -type l
  echo "[hz11-standalone] symlink found in HZ11 tree" >&2
  exit 1
fi

echo "[hz11-standalone] checking script executability"
if find scripts -type f -name '*.sh' ! -perm -u+x | grep -q .; then
  find scripts -type f -name '*.sh' ! -perm -u+x
  echo "[hz11-standalone] shell script without executable bit" >&2
  exit 1
fi

echo "[hz11-standalone] checking positioning boundary"
if ! rg -q "speed-first" README.md docs/HZ11_POSITIONING_L0.md; then
  echo "[hz11-standalone] HZ11 must be documented as speed-first" >&2
  exit 1
fi
if ! rg -q "does not replace HZ8|not HZ11" README.md docs/HZ11_POSITIONING_L0.md; then
  echo "[hz11-standalone] HZ11 must not be documented as replacing HZ8" >&2
  exit 1
fi

echo "[hz11-standalone] checking active file line limits"
if find "${CHECK_TREE[@]}" -type f \
    \( -name '*.c' -o -name '*.h' -o -name '*.sh' -o -name '*.md' \
       -o -name 'Makefile' \) -print0 |
    xargs -0 wc -l |
    awk '$2 != "total" && $1 > 800 { print; bad = 1 }
         END { exit bad ? 0 : 1 }'; then
  echo "[hz11-standalone] file over 800 lines" >&2
  exit 1
fi

echo "[hz11-standalone] ok"
