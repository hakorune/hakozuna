#!/usr/bin/env bash

HZ6_PRELOAD_SELECTED_BASE_CFLAGS=(
  -DHZ6_ROUTE_TABLE_CAPACITY=65536
  -DHZ6_OBJECT_DESCRIPTOR_CAPACITY=16384
  -DHZ6_SOURCE_BLOCK_CAPACITY=2048
  -DHZ6_FRONT_CACHE_BIN_CAPACITY=4096
  -DHZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=32768
  -DHZ6_TOY_SOURCE_BLOCK_BYTES=65536
  -DHZ6_MIDPAGE_RUN_BYTES=262144
  -DHZ6_MIDPAGE_32K_RUN_BYTES=786432
  -DHZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
  -DHZ6_MIDPAGE_ACTIVE_FREE_MAP_EXTERNAL_L2=1
  -DHZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
  -DHZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
  -DHZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
  -DHZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1
  -DHZ6_PRELOAD_REALLOC_IN_PLACE_L1=1
  -DHZ6_LINUX_MMAP_RETAIN_L1=1
  -DHZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1
  -DHZ6_TOY_FULL_BLOCK_PREFILL_L1=1
  -DHZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=128
  -DHZ6_ROUTE_TOMBSTONE_COMPACT_L1=1
  -DHZ6_ROUTE_HASH_XOR_FOLD_L1=1
  -DHZ6_ROUTE_LINEAR_WRAP_L1=1
  -DHZ6_ROUTE_LOOP_CARRY_L1=1
  -DHZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1

  # Explicit no-go/control defaults. Override with runner variants for A/B.
  -DHZ6_LINUX_MMAP_RETAIN_TLS_L1=0
  -DHZ6_SOURCE_RUN_REUSE_L1=0
  -DHZ6_ROUTE_PACKED_META_L1=0
  -DHZ6_PRELOAD_FAST_FREE_L1=0
)

HZ6_PRELOAD_MIDPAGE_BOUNDARY_CFLAGS=(
  -DHZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1
  -DHZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1=1
)

hz6_preload_effective_selected_cflags() {
  local -n out_ref="$1"
  local enable_midpage_boundary="${2:-1}"
  out_ref=("${HZ6_PRELOAD_SELECTED_BASE_CFLAGS[@]}")
  if [[ "$enable_midpage_boundary" -ne 0 ]]; then
    out_ref+=("${HZ6_PRELOAD_MIDPAGE_BOUNDARY_CFLAGS[@]}")
  fi
}

hz6_preload_replace_define() {
  local -n flags_ref="$1"
  local macro="$2"
  local value="$3"
  local define="-D${macro}=${value}"
  local i
  for i in "${!flags_ref[@]}"; do
    if [[ "${flags_ref[$i]}" == "-D${macro}="* ]]; then
      flags_ref[$i]="$define"
      return
    fi
  done
  flags_ref+=("$define")
}

hz6_preload_join_flags() {
  local out=""
  local flag
  for flag in "$@"; do
    out+="${flag} "
  done
  printf '%s' "$out"
}
