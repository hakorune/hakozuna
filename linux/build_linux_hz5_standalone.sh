#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="auto"
OUT_DIR=""
ENABLE_LINUX_P43=0
LINUX_LOCAL2P=0
LINUX_LOCAL2P_TLS_PACKED=0
LINUX_LOCAL2P_TLS_INITIAL_EXEC=0
LINUX_LOCAL2P_DIRECT_ROUTE=0
LINUX_LOCAL2P_DIRECT_INIT=0
LINUX_LOCAL2P_NO_COOKIE=0
LINUX_LOCAL2P_NO_CAS=0
LINUX_LOCAL2P_OWNER_INBOX=0
LINUX_LOCAL2P_OBJECT_NODE=0
LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=0
LINUX_LOCAL2P_ROUTE_COOKIE=0
LINUX_LOCAL2P_REUSE_STATE_ONLY=0
LINUX_P25_BRIDGE_ATTR=0
LINUX_P25_BRIDGE_ATTR_NO_CAS=0
LINUX_P25_BRIDGE_ATTR_NO_COOKIE=0
LINUX_P25_BRIDGE_ATTR_READONLY_STATE=0
LINUX_P43_PREPARED_BRIDGE=1
LINUX_P43_UNSAFE_NO_LOOKUP=0
LINUX_P43_TRUST_FAST_LOOKUP=0
LINUX_P43_TRUST_WRAPPER_SOURCE=0
LINUX_P43_DECODED_RAW_LOOKUP=0
LINUX_P43_RAW_FAST_LOOKUP_ONLY=0
LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY=0
LINUX_P43_WRAPPER_TOKEN=0
LINUX_P43_WRAPPER_TOKEN_BRIDGE=0
TRACE_LANE=0

usage() {
  cat <<'EOF'
Usage:
  ./linux/build_linux_hz5_standalone.sh [options]

Options:
  --arch <arch>      override detected arch (default: auto)
  --out-dir DIR      output directory (default: hakozuna-hz5/out/linux/<arch>)
  --linux-local2p    enable Linux Local2P exact 64K/a8192 TLS span candidate
  --linux-local2p-tls-packed
                     pack Local2P TLS state into one TLS object
  --linux-local2p-initial-exec
                     build with -ftls-model=initial-exec for Local2P speed A/B
  --linux-local2p-direct-route
                     test Local2P exact 64K/a8192 before generic route helper
  --linux-local2p-direct-init
                     initialize Local2P wrapper prefix directly
  --linux-local2p-fast
                     enable packed TLS, initial-exec, direct route, direct init
  --linux-local2p-no-cookie
                     diagnostic only: skip Local2P attribution cookie recompute
  --linux-local2p-no-cas
                     diagnostic only: replace Local2P ACTIVE->FREED CAS with load/store
  --linux-local2p-owner-inbox
                     candidate only: route remote frees to owner MPSC inbox
  --linux-local2p-object-node
                     candidate only: use aligned user pointers as Local2P free-list nodes
  --linux-local2p-same-owner-fast-state
                     candidate only: object-node with owner-local load/store state transition
  --linux-local2p-route-cookie
                     candidate only: fast-state lane using Local2P cookie as direct route guard
  --linux-local2p-reuse-state-only
                     candidate only: route-cookie lane updating only state on TLS reuse
  --linux-p25-bridge-attr
                     preserve P25 bridge topology with wrapper attr CAS guard
  --linux-p25-bridge-attr-no-cas
                     diagnostic only: validate bridge attr with load/store, no CAS
  --linux-p25-bridge-attr-no-cookie
                     diagnostic only: bridge attr CAS without cookie recompute
  --linux-p25-bridge-attr-readonly-state
                     diagnostic only: validate state but do not mark it freed
  --linux-p43        enable Linux P43 segment-slot source candidate lane
  --linux-p43-no-prepared-bridge
                     disable P43 PreparedBridge for source-only A/B
  --linux-p43-unsafe-no-lookup
                     bypass P43 free lookup for lookup-cost A/B
  --linux-p43-trust-fast-lookup
                     trust P43 fast lookup hits as active for candidate A/B
  --linux-p43-trust-wrapper-source
                     trust decoded P25 wrapper source and skip P43 lookup
  --linux-p43-decoded-raw-lookup
                     decode wrapper first, then validate raw through P43 lookup
  --linux-p43-decoded-raw-fastlookup
                     decoded raw lookup uses only P43 fast table and masks
  --linux-p43-decoded-raw-allocated
                     decoded raw lookup uses fast table and allocated bit only
  --linux-p43-token  store P43 segment/slot token in the HZ5 wrapper
  --linux-p43-token-bridge
                     validate wrapper token, then release through P25 bridge
  --trace-lane       enable route/reuse counters; disables SPEED_LANE
  --help             show this message
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch)
      [[ $# -ge 2 ]] || { echo "missing value for --arch" >&2; exit 1; }
      ARCH="$2"
      shift 2
      ;;
    --out-dir)
      [[ $# -ge 2 ]] || { echo "missing value for --out-dir" >&2; exit 1; }
      OUT_DIR="$2"
      shift 2
      ;;
    --linux-local2p)
      LINUX_LOCAL2P=1
      shift
      ;;
    --linux-local2p-tls-packed)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      shift
      ;;
    --linux-local2p-initial-exec)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      shift
      ;;
    --linux-local2p-direct-route)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      shift
      ;;
    --linux-local2p-direct-init)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      shift
      ;;
    --linux-local2p-fast)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      shift
      ;;
    --linux-local2p-no-cookie)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_NO_COOKIE=1
      shift
      ;;
    --linux-local2p-no-cas)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_NO_CAS=1
      shift
      ;;
    --linux-local2p-owner-inbox)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_OWNER_INBOX=1
      shift
      ;;
    --linux-local2p-object-node)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      shift
      ;;
    --linux-local2p-same-owner-fast-state)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      shift
      ;;
    --linux-local2p-route-cookie)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      LINUX_LOCAL2P_ROUTE_COOKIE=1
      shift
      ;;
    --linux-local2p-reuse-state-only)
      LINUX_LOCAL2P=1
      LINUX_LOCAL2P_TLS_PACKED=1
      LINUX_LOCAL2P_TLS_INITIAL_EXEC=1
      LINUX_LOCAL2P_DIRECT_ROUTE=1
      LINUX_LOCAL2P_DIRECT_INIT=1
      LINUX_LOCAL2P_OBJECT_NODE=1
      LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1
      LINUX_LOCAL2P_ROUTE_COOKIE=1
      LINUX_LOCAL2P_REUSE_STATE_ONLY=1
      shift
      ;;
    --linux-p25-bridge-attr)
      LINUX_P25_BRIDGE_ATTR=1
      shift
      ;;
    --linux-p25-bridge-attr-no-cas)
      LINUX_P25_BRIDGE_ATTR=1
      LINUX_P25_BRIDGE_ATTR_NO_CAS=1
      shift
      ;;
    --linux-p25-bridge-attr-no-cookie)
      LINUX_P25_BRIDGE_ATTR=1
      LINUX_P25_BRIDGE_ATTR_NO_COOKIE=1
      shift
      ;;
    --linux-p25-bridge-attr-readonly-state)
      LINUX_P25_BRIDGE_ATTR=1
      LINUX_P25_BRIDGE_ATTR_READONLY_STATE=1
      shift
      ;;
    --linux-p43)
      ENABLE_LINUX_P43=1
      shift
      ;;
    --linux-p43-no-prepared-bridge)
      ENABLE_LINUX_P43=1
      LINUX_P43_PREPARED_BRIDGE=0
      shift
      ;;
    --linux-p43-unsafe-no-lookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_UNSAFE_NO_LOOKUP=1
      shift
      ;;
    --linux-p43-trust-fast-lookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_TRUST_FAST_LOOKUP=1
      shift
      ;;
    --linux-p43-trust-wrapper-source)
      ENABLE_LINUX_P43=1
      LINUX_P43_TRUST_WRAPPER_SOURCE=1
      shift
      ;;
    --linux-p43-decoded-raw-lookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_DECODED_RAW_LOOKUP=1
      shift
      ;;
    --linux-p43-decoded-raw-fastlookup)
      ENABLE_LINUX_P43=1
      LINUX_P43_DECODED_RAW_LOOKUP=1
      LINUX_P43_RAW_FAST_LOOKUP_ONLY=1
      shift
      ;;
    --linux-p43-decoded-raw-allocated)
      ENABLE_LINUX_P43=1
      LINUX_P43_DECODED_RAW_LOOKUP=1
      LINUX_P43_RAW_FAST_LOOKUP_ONLY=1
      LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY=1
      shift
      ;;
    --linux-p43-token)
      ENABLE_LINUX_P43=1
      LINUX_P43_WRAPPER_TOKEN=1
      shift
      ;;
    --linux-p43-token-bridge)
      ENABLE_LINUX_P43=1
      LINUX_P43_WRAPPER_TOKEN=1
      LINUX_P43_WRAPPER_TOKEN_BRIDGE=1
      shift
      ;;
    --trace-lane)
      TRACE_LANE=1
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ "$LINUX_LOCAL2P" -eq 1 && \
      ( "$LINUX_P25_BRIDGE_ATTR" -eq 1 || "$ENABLE_LINUX_P43" -eq 1 ) ]]; then
  echo "local2p, p25attr, and p43 lanes are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_LOCAL2P_NO_COOKIE" -eq 1 && \
      "$LINUX_LOCAL2P_NO_CAS" -eq 1 ]]; then
  echo "local2p diagnostic variants are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 && "$ENABLE_LINUX_P43" -eq 1 ]]; then
  echo "p25attr and p43 lanes are mutually exclusive" >&2
  exit 1
fi

if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 ]]; then
  p25attr_mode_count=0
  [[ "$LINUX_P25_BRIDGE_ATTR_NO_CAS" -eq 1 ]] && ((p25attr_mode_count += 1))
  [[ "$LINUX_P25_BRIDGE_ATTR_NO_COOKIE" -eq 1 ]] && ((p25attr_mode_count += 1))
  [[ "$LINUX_P25_BRIDGE_ATTR_READONLY_STATE" -eq 1 ]] && ((p25attr_mode_count += 1))
  if [[ "$p25attr_mode_count" -gt 1 ]]; then
    echo "p25attr diagnostic variants are mutually exclusive" >&2
    exit 1
  fi
fi

if [[ "$ENABLE_LINUX_P43" -eq 1 ]]; then
  p43_mode_count=0
  [[ "$LINUX_P43_PREPARED_BRIDGE" -eq 0 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_UNSAFE_NO_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_TRUST_FAST_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_TRUST_WRAPPER_SOURCE" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 1 ]] && ((p43_mode_count += 1))
  [[ "$LINUX_P43_WRAPPER_TOKEN" -eq 1 ]] && ((p43_mode_count += 1))
  if [[ "$p43_mode_count" -gt 1 ]]; then
    echo "P43 lane selectors are mutually exclusive" >&2
    exit 1
  fi
  if [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 0 && \
        ( "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 || \
          "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ) ]]; then
    echo "rawlookup submodes require --linux-p43-decoded-raw-lookup" >&2
    exit 1
  fi
  if [[ "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 && \
        "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ]]; then
    echo "rawlookup fast/allocated submodes are mutually exclusive" >&2
    exit 1
  fi
  if [[ "$LINUX_P43_WRAPPER_TOKEN_BRIDGE" -eq 1 && \
        "$LINUX_P43_WRAPPER_TOKEN" -eq 0 ]]; then
    echo "--linux-p43-token-bridge requires --linux-p43-token" >&2
    exit 1
  fi
fi

if [[ "$ARCH" == "auto" ]]; then
  case "$(uname -m)" in
    aarch64|arm64) ARCH="arm64" ;;
    x86_64|amd64) ARCH="x86_64" ;;
    *) ARCH="$(uname -m)" ;;
  esac
fi

HZ5_DIR="${ROOT_DIR}/hakozuna-hz5"
SOURCE_COMMIT="$(git -C "$ROOT_DIR" rev-parse HEAD)"
OUT_DIR="${OUT_DIR:-${HZ5_DIR}/out/linux/${ARCH}}"
LIB="${OUT_DIR}/libhakozuna_hz5_standalone.so"
PRELOAD_HYBRID_LIB="${OUT_DIR}/libhakozuna_hz5_preload_hybrid.so"
BENCH="${OUT_DIR}/bench_hz5_standalone_aligned64k"
REMOTE_BENCH="${OUT_DIR}/bench_hz5_standalone_remote64k"
RSS_BENCH="${OUT_DIR}/bench_hz5_standalone_rss_plateau"
MIXED_BENCH="${OUT_DIR}/bench_hz5_standalone_mixed_prelude"
SAFETY_BENCH="${OUT_DIR}/bench_hz5_standalone_safety"
GENERIC_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_aligned64k"
GENERIC_REMOTE_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_remote64k"
GENERIC_RSS_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_rss_plateau"
GENERIC_MIXED_BENCH="${ROOT_DIR}/bench/out/linux/${ARCH}/bench_mixed_prelude"
BUILD_CONFIG="${OUT_DIR}/hz5_build_config.env"
SPEED_LANE=1
if [[ "$TRACE_LANE" -eq 1 ]]; then
  SPEED_LANE=0
fi

mkdir -p "$OUT_DIR"

COMMON_FLAGS=(
  -O3
  -fPIC
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-function
  -Wno-unused-variable
  -std=c11
  -D_GNU_SOURCE
  -DNDEBUG
  -DHZ5_DIAGNOSTIC_STATS=0
  -DBENCHLAB_HZ5_SPEED_LANE="${SPEED_LANE}"
  -DBENCHLAB_HZ5_TRACE_LANE="${TRACE_LANE}"
  -DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1
  -DBENCHLAB_HZ5_STANDALONE_EXACT_ONLY=1
  -DBENCHLAB_HZ5_P25_HZ4LOWPAGE64K_A8192=1
  -DBENCHLAB_HZ5_P25_STATS=0
  -DHZ5_DESC_SOURCE_COMMIT=\"${SOURCE_COMMIT}\"
  -I"${HZ5_DIR}/include"
  -I"${HZ5_DIR}/contract"
  -I"${HZ5_DIR}/policy"
  -I"${HZ5_DIR}/route"
  -I"${HZ5_DIR}/wrapper"
  -I"${HZ5_DIR}/lowpage"
  -I"${HZ5_DIR}/fallback"
)

if [[ "$LINUX_P25_BRIDGE_ATTR" -eq 1 ]]; then
  COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR=1)
  if [[ "$LINUX_P25_BRIDGE_ATTR_NO_CAS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS=1)
  fi
  if [[ "$LINUX_P25_BRIDGE_ATTR_NO_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE=1)
  fi
  if [[ "$LINUX_P25_BRIDGE_ATTR_READONLY_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE=1)
  fi
fi

if [[ "$LINUX_LOCAL2P" -eq 1 ]]; then
  COMMON_FLAGS+=(
    -DBENCHLAB_HZ5_LINUX_LOCAL2P=1
    -DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_CAP=1
  )
  if [[ "$LINUX_LOCAL2P_TLS_PACKED" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED=1)
  fi
  if [[ "$LINUX_LOCAL2P_DIRECT_ROUTE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE=1)
  fi
  if [[ "$LINUX_LOCAL2P_DIRECT_INIT" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT=1)
  fi
  if [[ "$LINUX_LOCAL2P_TLS_INITIAL_EXEC" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_TLS_INITIAL_EXEC=1)
    COMMON_FLAGS+=(-ftls-model=initial-exec)
  fi
  if [[ "$LINUX_LOCAL2P_NO_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_NO_COOKIE=1)
  fi
  if [[ "$LINUX_LOCAL2P_NO_CAS" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_NO_CAS=1)
  fi
  if [[ "$LINUX_LOCAL2P_OWNER_INBOX" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_OWNER_INBOX=1)
  fi
  if [[ "$LINUX_LOCAL2P_OBJECT_NODE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_OBJECT_NODE=1)
  fi
  if [[ "$LINUX_LOCAL2P_SAME_OWNER_FAST_STATE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_SAME_OWNER_FAST_STATE=1)
  fi
  if [[ "$LINUX_LOCAL2P_ROUTE_COOKIE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_ROUTE_COOKIE=1)
  fi
  if [[ "$LINUX_LOCAL2P_REUSE_STATE_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_LINUX_LOCAL2P_REUSE_STATE_ONLY=1)
  fi
fi

if [[ "$ENABLE_LINUX_P43" -eq 1 ]]; then
  COMMON_FLAGS+=(
    -DBENCHLAB_HZ5_LINUX_P43_PORT=1
    -DBENCHLAB_HZ5_P43_SEGMENT_SLOTS=1
    -DBENCHLAB_HZ5_P43_FAST_LOOKUP=1
    -DBENCHLAB_HZ5_P43_LOCKLESS_LOOKUP=1
    -DBENCHLAB_HZ5_P43_SLOT_DECOMMIT=0
    -DBENCHLAB_HZ5_P43_PAGE_NOACCESS=0
  )
  if [[ "$LINUX_P43_PREPARED_BRIDGE" -eq 1 ]]; then
    COMMON_FLAGS+=(
      -DBENCHLAB_HZ5_P43_PREPARED_BRIDGE=1
      -DBENCHLAB_HZ5_P43_LOCKLESS_CONTRACT=1
    )
  fi
  if [[ "$LINUX_P43_UNSAFE_NO_LOOKUP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_UNSAFE_NO_LOOKUP=1)
  fi
  if [[ "$LINUX_P43_TRUST_FAST_LOOKUP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_TRUST_FAST_LOOKUP=1)
  fi
  if [[ "$LINUX_P43_TRUST_WRAPPER_SOURCE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_TRUST_WRAPPER_SOURCE=1)
  fi
  if [[ "$LINUX_P43_DECODED_RAW_LOOKUP" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_DECODED_RAW_LOOKUP=1)
  fi
  if [[ "$LINUX_P43_RAW_FAST_LOOKUP_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_RAW_FAST_LOOKUP_ONLY=1)
  fi
  if [[ "$LINUX_P43_RAW_ALLOCATED_LOOKUP_ONLY" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_RAW_ALLOCATED_LOOKUP_ONLY=1)
  fi
  if [[ "$LINUX_P43_WRAPPER_TOKEN" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_WRAPPER_TOKEN=1)
  fi
  if [[ "$LINUX_P43_WRAPPER_TOKEN_BRIDGE" -eq 1 ]]; then
    COMMON_FLAGS+=(-DBENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE=1)
  fi
fi

{
  echo "commit=${SOURCE_COMMIT}"
  if [[ -n "$(git -C "$ROOT_DIR" status --porcelain --untracked-files=all)" ]]; then
    echo "dirty=1"
  else
    echo "dirty=0"
  fi
  echo "arch=${ARCH}"
  echo "trace_lane=${TRACE_LANE}"
  echo "speed_lane=${SPEED_LANE}"
  echo "linux_local2p=${LINUX_LOCAL2P}"
  echo "linux_local2p_tls_packed=${LINUX_LOCAL2P_TLS_PACKED}"
  echo "linux_local2p_tls_initial_exec=${LINUX_LOCAL2P_TLS_INITIAL_EXEC}"
  echo "linux_local2p_direct_route=${LINUX_LOCAL2P_DIRECT_ROUTE}"
  echo "linux_local2p_direct_init=${LINUX_LOCAL2P_DIRECT_INIT}"
  echo "linux_local2p_no_cookie=${LINUX_LOCAL2P_NO_COOKIE}"
  echo "linux_local2p_no_cas=${LINUX_LOCAL2P_NO_CAS}"
  echo "linux_local2p_owner_inbox=${LINUX_LOCAL2P_OWNER_INBOX}"
  echo "linux_local2p_object_node=${LINUX_LOCAL2P_OBJECT_NODE}"
  echo "linux_local2p_same_owner_fast_state=${LINUX_LOCAL2P_SAME_OWNER_FAST_STATE}"
  echo "linux_local2p_route_cookie=${LINUX_LOCAL2P_ROUTE_COOKIE}"
  echo "linux_local2p_reuse_state_only=${LINUX_LOCAL2P_REUSE_STATE_ONLY}"
  echo "linux_p25_bridge_attr=${LINUX_P25_BRIDGE_ATTR}"
  echo "linux_p25_bridge_attr_no_cas=${LINUX_P25_BRIDGE_ATTR_NO_CAS}"
  echo "linux_p25_bridge_attr_no_cookie=${LINUX_P25_BRIDGE_ATTR_NO_COOKIE}"
  echo "linux_p25_bridge_attr_readonly_state=${LINUX_P25_BRIDGE_ATTR_READONLY_STATE}"
  echo "enable_linux_p43=${ENABLE_LINUX_P43}"
  echo "linux_p43_prepared_bridge=${LINUX_P43_PREPARED_BRIDGE}"
  echo "linux_p43_trust_wrapper_source=${LINUX_P43_TRUST_WRAPPER_SOURCE}"
  echo "linux_p43_wrapper_token=${LINUX_P43_WRAPPER_TOKEN}"
  echo "linux_p43_wrapper_token_bridge=${LINUX_P43_WRAPPER_TOKEN_BRIDGE}"
} > "$BUILD_CONFIG"

HZ5_SRCS=(
  "${HZ5_DIR}/api/hz5_api.c"
  "${HZ5_DIR}/contract/hz5_contract.c"
  "${HZ5_DIR}/core/hz5_segment.c"
  "${HZ5_DIR}/core/hz5_run.c"
  "${HZ5_DIR}/core/hz5_owner.c"
  "${HZ5_DIR}/core/hz5_remote.c"
  "${HZ5_DIR}/core/hz5_tcache.c"
  "${HZ5_DIR}/core/hz5_stats.c"
  "${HZ5_DIR}/policy/hz5_policy.c"
  "${HZ5_DIR}/policy/hz5_trace.c"
  "${HZ5_DIR}/route/hz5_route.c"
  "${HZ5_DIR}/wrapper/hz5_wrapper.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64_os.c"
  "${HZ5_DIR}/lowpage/hz5_lowpage64_p43_segment.c"
  "${HZ5_DIR}/linux/hz5_linux_compat.c"
)

echo "[linux][hz5] arch: ${ARCH}"
echo "[linux][hz5] building library: ${LIB}"
gcc "${COMMON_FLAGS[@]}" -shared "${HZ5_SRCS[@]}" -pthread -ldl -o "$LIB"

echo "[linux][hz5] building preload hybrid library: ${PRELOAD_HYBRID_LIB}"
gcc "${COMMON_FLAGS[@]}" -shared \
  "${HZ5_SRCS[@]}" \
  "${HZ5_DIR}/preload/hz5_preload_hybrid.c" \
  -pthread -ldl -o "$PRELOAD_HYBRID_LIB"

echo "[linux][hz5] building benchmark: ${BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_aligned64k.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$BENCH"

echo "[linux][hz5] building remote benchmark: ${REMOTE_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_remote64k.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$REMOTE_BENCH"

echo "[linux][hz5] building RSS plateau benchmark: ${RSS_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_rss_plateau.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$RSS_BENCH"

echo "[linux][hz5] building mixed prelude benchmark: ${MIXED_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  -I"${HZ5_DIR}/include" \
  "${ROOT_DIR}/bench/bench_hz5_standalone_mixed_prelude.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$MIXED_BENCH"

echo "[linux][hz5] building safety benchmark: ${SAFETY_BENCH}"
gcc "${COMMON_FLAGS[@]}" -Werror -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_hz5_standalone_safety.c" \
  -L"${OUT_DIR}" -Wl,-rpath,"${OUT_DIR}" -lhakozuna_hz5_standalone \
  -pthread -o "$SAFETY_BENCH"

mkdir -p "$(dirname "$GENERIC_BENCH")"
echo "[linux][hz5] building generic aligned benchmark: ${GENERIC_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_aligned64k.c" \
  -pthread -o "$GENERIC_BENCH"

echo "[linux][hz5] building generic remote benchmark: ${GENERIC_REMOTE_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_remote64k.c" \
  -pthread -o "$GENERIC_REMOTE_BENCH"

echo "[linux][hz5] building generic RSS plateau benchmark: ${GENERIC_RSS_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_rss_plateau.c" \
  -pthread -o "$GENERIC_RSS_BENCH"

echo "[linux][hz5] building generic mixed prelude benchmark: ${GENERIC_MIXED_BENCH}"
gcc -O3 -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
  "${ROOT_DIR}/bench/bench_mixed_prelude.c" \
  -pthread -o "$GENERIC_MIXED_BENCH"

echo "[linux][hz5] done"
