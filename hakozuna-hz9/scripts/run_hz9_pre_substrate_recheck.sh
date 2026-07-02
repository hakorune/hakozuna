#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

RUN_SMOKE="${RUN_SMOKE:-1}"
RUN_BUILDS="${RUN_BUILDS:-1}"
RUN_LAYOUT="${RUN_LAYOUT:-1}"
RUN_SHAPE="${RUN_SHAPE:-1}"
RUN_PROBE="${RUN_PROBE:-0}"

echo "[hz9-pre-substrate] standalone check"
make -C "$ROOT" hz9-standalone-check

if [[ "$RUN_SMOKE" != "0" ]]; then
  echo "[hz9-pre-substrate] localarena smoke"
  make -C "$ROOT" smoke-hz9localarena
  echo "[hz9-pre-substrate] owner page route smoke"
  make -C "$ROOT" smoke-hz9ownerpagepool-route
fi

if [[ "$RUN_BUILDS" != "0" ]]; then
  echo "[hz9-pre-substrate] remote-safe closeout builds"
  make -C "$ROOT" \
    bench-release-hz9localarena-dense-ownerfast-remotesafe \
    bench-hz9localarena-dense-ownerfast-remotesafe

  echo "[hz9-pre-substrate] integrated SlabPage closeout builds"
  make -C "$ROOT" \
    bench-hz9mediumslabpage-classes-min4-integrated-localfast-adaptive \
    bench-release-hz9mediumslabpage-classes-min4-integrated-localfast-adaptive \
    bench-hz9mediumslabpage-classes-min4-integrated-layout-neutral-proof \
    bench-release-hz9mediumslabpage-classes-min4-integrated-layout-neutral-proof

  echo "[hz9-pre-substrate] owner-local page pool scaffold/shadow builds"
  make -C "$ROOT" \
    bench-hz9ownerpagepool-scaffold \
    bench-release-hz9ownerpagepool-scaffold \
    bench-hz9ownerpagepool-shadow \
    bench-release-hz9ownerpagepool-shadow \
    bench-hz9ownerpagepool-purelocal-api \
    bench-release-hz9ownerpagepool-purelocal-api
fi

if [[ "$RUN_LAYOUT" != "0" ]]; then
  echo "[hz9-pre-substrate] layout-neutral proof audit"
  VARIANTS=baseline,slabintegrated_min4_layoutneutral,ownerpagepool_scaffold \
    "$ROOT/scripts/run_hz9_layout_audit.sh"
fi

if [[ "$RUN_SHAPE" != "0" ]]; then
  echo "[hz9-pre-substrate] owner-local page pool code-shape audit"
  MODE=ownerpool "$ROOT/scripts/run_hz9_code_shape_audit.sh"
fi

if [[ "$RUN_PROBE" != "0" ]]; then
  echo "[hz9-pre-substrate] optional integrated SlabPage R1 probe"
  RUNS="${RUNS:-1}" THREADS="${THREADS:-8}" ITERS="${ITERS:-30000}" \
    ROWS="${ROWS:-main_local0,medium_local0,medium_r50,main_r90,small_interleaved_remote90}" \
    VARIANTS="${VARIANTS:-baseline,slabintegrated_min4}" \
    "$ROOT/scripts/run_hz9_candidate_gate.sh"
fi

echo "[hz9-pre-substrate] ok"
