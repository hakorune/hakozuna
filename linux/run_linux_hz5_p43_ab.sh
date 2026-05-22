#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
RUNS=5
THREADS=1
ITERS=1000000
SIZE=65536
ALIGN=8192
OUTDIR="${ROOT_DIR}/private/raw-results/linux/hz5_p43_ab_$(date +%Y%m%d_%H%M%S)"
SKIP_BUILD=0

usage() {
  cat <<'EOF'
Usage:
  ./linux/run_linux_hz5_p43_ab.sh [options]

Options:
  --arch <arch>      override detected arch (default: auto)
  --runs N           runs per lane (default: 5)
  --threads N        worker threads (default: 1)
  --iters N          iterations per thread (default: 1000000)
  --size N           allocation size (default: 65536)
  --align N          allocation alignment (default: 8192)
  --outdir DIR       output directory
  --skip-build       reuse existing binaries
  --help             show this message

Lanes:
  p25           baseline P25 lowpage64 speed lane
  p43           Linux P43 segment-slot source + PreparedBridge + lookup
  p43-trustfast same as p43, but fast lookup hits are trusted as active
  p43-trustwrap same as p43, but decoded P25 wrapper source skips lookup
  p43-rawlookup same as p43, but decoded raw is validated by P43 lookup
  p43-rawfast   same as rawlookup, but raw validation uses fast table only
  p43-rawalloc  same as rawfast, but active check uses allocated bit only
  p43-token     stores P43 segment/slot token in wrapper and direct-releases it
  p43-tokenbridge validates wrapper token, then releases through P25 bridge
  p43-nolookup  same as p43, but bypasses P43 free lookup
  p43-source    Linux P43 segment-slot source without PreparedBridge
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --threads) THREADS="$2"; shift 2 ;;
    --iters) ITERS="$2"; shift 2 ;;
    --size) SIZE="$2"; shift 2 ;;
    --align) ALIGN="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --help|-h) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

mkdir -p "$OUTDIR"

if [[ "$SKIP_BUILD" -ne 1 ]]; then
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p25"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-trust-fast-lookup \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-trustfast"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-trust-wrapper-source \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-trustwrap"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-decoded-raw-lookup \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-rawlookup"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-decoded-raw-fastlookup \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-rawfast"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-decoded-raw-allocated \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-rawalloc"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-token \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-token"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-token-bridge \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-tokenbridge"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43 --linux-p43-unsafe-no-lookup \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-nolookup"
  "${ROOT_DIR}/linux/build_linux_hz5_standalone.sh" \
    --arch "$ARCH" --linux-p43-no-prepared-bridge \
    --out-dir "${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-p43-source"
fi

{
  echo "commit=$(git -C "$ROOT_DIR" rev-parse HEAD)"
  echo "date=$(date -Is)"
  echo "arch=${ARCH}"
  echo "runs=${RUNS}"
  echo "threads=${THREADS}"
  echo "iters=${ITERS}"
  echo "size=${SIZE}"
  echo "align=${ALIGN}"
  echo "note=p43-trustfast isolates mask-load cost on fast hits; p43-trustwrap skips lookup after wrapper decode; p43-rawlookup validates decoded raw through P43 lookup; p43-rawfast validates decoded raw through fast table and masks only; p43-rawalloc validates decoded raw through fast table and allocated bit only; p43-token stores segment/slot in wrapper and direct-releases with allocated-bit guard; p43-tokenbridge validates wrapper token but preserves P25 bridge release topology; p43-nolookup isolates full free-lookup cost; p43-source isolates segment-source cost without PreparedBridge."
} > "${OUTDIR}/README.log"

printf 'lane\trun\tstatus\tthreads\titers\tsize\talign\tops_s\tru_maxrss_kb\tlog\n' \
  > "${OUTDIR}/results.tsv"

run_lane() {
  local lane="$1"
  local run="$2"
  local bin="${ROOT_DIR}/hakozuna-hz5/out/linux/${ARCH}-${lane}/bench_hz5_standalone_aligned64k"
  local log="${OUTDIR}/${lane}_${run}.log"
  local timefile="${OUTDIR}/${lane}_${run}.time"
  local status=0

  /usr/bin/time -f 'ru_maxrss_kb=%M' -o "$timefile" \
    "$bin" "$THREADS" "$ITERS" "$SIZE" "$ALIGN" > "$log" 2>&1 || status=$?

  local ops_s
  ops_s="$(awk -F'ops/s=' '/ops\/s=/{v=$2} END{gsub(/[[:space:]].*/, "", v); print v}' "$log")"
  local rss
  rss="$(awk -F= '/ru_maxrss_kb=/{print $2}' "$timefile" 2>/dev/null | tail -n 1)"

  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$lane" "$run" "$status" "$THREADS" "$ITERS" "$SIZE" "$ALIGN" \
    "${ops_s:-NA}" "${rss:-NA}" "$log" >> "${OUTDIR}/results.tsv"
}

for run in $(seq 1 "$RUNS"); do
  for lane in p25 p43 p43-trustfast p43-trustwrap p43-rawlookup p43-rawfast p43-rawalloc p43-token p43-tokenbridge p43-nolookup p43-source; do
    run_lane "$lane" "$run"
  done
done

awk -F'\t' '
  NR > 1 && $3 == 0 && $8 != "NA" {
    key = $1
    ops[key, ++n[key]] = $8 + 0
    rss[key, n[key]] = $9 + 0
  }
  END {
    print "lane\truns\tmedian_ops_s\tmedian_ru_maxrss_kb"
    for (key in n) {
      for (i = 1; i <= n[key]; ++i) {
        so[i] = ops[key, i]
        sr[i] = rss[key, i]
      }
      for (i = 1; i <= n[key]; ++i) {
        for (j = i + 1; j <= n[key]; ++j) {
          if (so[j] < so[i]) { t = so[i]; so[i] = so[j]; so[j] = t }
          if (sr[j] < sr[i]) { t = sr[i]; sr[i] = sr[j]; sr[j] = t }
        }
      }
      mid = int((n[key] + 1) / 2)
      print key "\t" n[key] "\t" so[mid] "\t" sr[mid]
      for (i = 1; i <= n[key]; ++i) {
        delete so[i]
        delete sr[i]
      }
    }
  }
' "${OUTDIR}/results.tsv" | sort > "${OUTDIR}/summary.tsv"

echo "[SUMMARY] ${OUTDIR}/summary.tsv"
echo "[DONE] logs saved to ${OUTDIR}"
