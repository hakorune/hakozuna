#!/usr/bin/env bash
set -euo pipefail

bench_os_name() {
  uname -s
}

bench_is_macos() {
  [[ "$(bench_os_name)" == "Darwin" ]]
}

bench_is_linux() {
  [[ "$(bench_os_name)" == "Linux" ]]
}

bench_preload_var() {
  if bench_is_macos; then
    printf '%s\n' "DYLD_INSERT_LIBRARIES"
  else
    printf '%s\n' "LD_PRELOAD"
  fi
}

bench_find_first_existing() {
  local candidate
  for candidate in "$@"; do
    [[ -n "$candidate" && -f "$candidate" ]] && {
      printf '%s\n' "$candidate"
      return 0
    }
  done
  return 1
}

bench_find_from_ldconfig() {
  local pattern="$1"
  if command -v ldconfig >/dev/null 2>&1; then
    ldconfig -p 2>/dev/null | rg "${pattern}" | head -n 1 | awk '{print $NF}' || true
  fi
}

bench_find_from_brew_prefix() {
  local formula="$1"
  shift
  local prefix=""
  if command -v brew >/dev/null 2>&1; then
    prefix="$(brew --prefix "${formula}" 2>/dev/null || true)"
  fi
  if [[ -n "${prefix}" ]]; then
    bench_find_first_existing "${prefix}/lib/$1" "${prefix}/lib/$2" "${prefix}/lib/$3" "${prefix}/lib/$4"
  fi
}

bench_find_private_linux_library() {
  local allocator="$1"
  local pattern="$2"
  local cache_root="${ROOT_DIR}/private/bench-assets/linux/allocators"

  if [[ -d "${cache_root}" ]]; then
    find "${cache_root}" -type f -path "*/${allocator}/*" -name "${pattern}" 2>/dev/null | sort | head -n 1
  fi
}

bench_find_mimalloc_library() {
  if bench_is_macos; then
    bench_find_first_existing \
      "${MIMALLOC_SO:-}" \
      "$(bench_find_from_brew_prefix mimalloc libmimalloc.dylib libmimalloc.so libmimalloc_minimal.dylib libmimalloc_minimal.so || true)" \
      "/opt/homebrew/opt/mimalloc/lib/libmimalloc.dylib" \
      "/usr/local/opt/mimalloc/lib/libmimalloc.dylib"
    return $?
  fi

  bench_find_first_existing \
    "${MIMALLOC_SO:-}" \
    "$(bench_find_from_ldconfig 'libmimalloc\\.so' || true)" \
    "$(bench_find_private_linux_library libmimalloc2.0 'libmimalloc.so*' || true)"
}

bench_find_tcmalloc_library() {
  if bench_is_macos; then
    bench_find_first_existing \
      "${TCMALLOC_SO:-}" \
      "$(bench_find_from_brew_prefix gperftools libtcmalloc.dylib libtcmalloc_minimal.dylib libtcmalloc.so libtcmalloc_minimal.so || true)" \
      "/opt/homebrew/opt/gperftools/lib/libtcmalloc.dylib" \
      "/opt/homebrew/opt/gperftools/lib/libtcmalloc_minimal.dylib" \
      "/usr/local/opt/gperftools/lib/libtcmalloc.dylib" \
      "/usr/local/opt/gperftools/lib/libtcmalloc_minimal.dylib"
    return $?
  fi

  bench_find_first_existing \
    "${TCMALLOC_SO:-}" \
    "$(bench_find_from_ldconfig 'libtcmalloc_minimal\\.so' || true)" \
    "$(bench_find_from_ldconfig 'libtcmalloc\\.so' || true)" \
    "$(bench_find_private_linux_library libtcmalloc-minimal4 'libtcmalloc_minimal.so*' || true)" \
    "$(bench_find_private_linux_library libtcmalloc-minimal4t64 'libtcmalloc_minimal.so*' || true)" \
    "$(bench_find_private_linux_library libgoogle-perftools4 'libtcmalloc.so*' || true)" \
    "$(bench_find_private_linux_library libgoogle-perftools4t64 'libtcmalloc.so*' || true)"
}

bench_find_hakorune_mimalloc_library() {
  local provider_dir="${HAKORUNE_MIMALLOC_PROVIDER_DIR:-${ROOT_DIR}/private/bench-assets/linux/allocators/hakorune-mimalloc-provider/hakorune-mimalloc-provider}"

  bench_find_first_existing \
    "${HAKORUNE_MIMALLOC_LDPRELOAD_SO:-}" \
    "${provider_dir}/libhakorune_provider_ldpreload.so" \
    "${ROOT_DIR}/hakorune-mimalloc-provider/libhakorune_provider_ldpreload.so"
}

bench_find_hz5_library() {
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    amd64) arch="x86_64" ;;
    arm64) arch="arm64" ;;
  esac

  bench_find_first_existing \
    "${HZ5_PRELOAD_FULL_SO:-}" \
    "${HZ5_SO:-}" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/${arch}-hz5-preload-full/libhakozuna_hz5_preload_full.so" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/${arch}-hz5-preload-full/libhakozuna_hz5_preload_hybrid.so" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/hz5-preload-full/libhakozuna_hz5_preload_full.so" \
    "${ROOT_DIR}/hakozuna-hz5/out/linux/hz5-preload-hybrid-smoke/libhakozuna_hz5_preload_hybrid.so"
}

bench_find_hz6_library() {
  bench_find_hz6_preload_output \
    HZ6_PRELOAD_SO \
    hz6_preload \
    hz6-preload
}

bench_find_hz8_library() {
  bench_find_first_existing \
    "${HZ8_SO:-}" \
    "${ROOT_DIR}/hakozuna-hz8/libhakozuna_hz8_preload.so"
}

bench_find_hz6_preload_output() {
  local env_var="$1"
  local out_dir="$2"
  local arch_suffix="$3"
  local override="${!env_var-}"
  local arch
  arch="$(uname -m)"
  case "${arch}" in
    amd64) arch="x86_64" ;;
    arm64) arch="arm64" ;;
  esac

  bench_find_first_existing \
    "${override}" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/${out_dir}/libhakozuna_hz6_preload.so" \
    "${ROOT_DIR}/hakozuna-hz6/out/linux/${arch}-${arch_suffix}/libhakozuna_hz6_preload.so"
}

bench_find_hz6_toy_target_library() {
  bench_find_hz6_preload_output \
    HZ6_TOY_TARGET_PRELOAD_SO \
    hz6_preload_toy_target \
    hz6-preload-toy-target
}

bench_find_hz6_toy_trusted_target_library() {
  bench_find_hz6_preload_output \
    HZ6_TOY_TRUSTED_TARGET_PRELOAD_SO \
    hz6_preload_toy_trusted_target \
    hz6-preload-toy-trusted-target
}

bench_find_hz6_aligned_target_library() {
  bench_find_hz6_preload_output \
    HZ6_ALIGNED_TARGET_PRELOAD_SO \
    hz6_preload_aligned_target \
    hz6-preload-aligned-target
}

bench_find_hz6_calloc_real_target_library() {
  bench_find_hz6_preload_output \
    HZ6_CALLOC_REAL_TARGET_PRELOAD_SO \
    hz6_preload_calloc_real_target \
    hz6-preload-calloc-real-target
}

bench_find_hz6_calloc_direct_target_library() {
  bench_find_hz6_preload_output \
    HZ6_CALLOC_DIRECT_TARGET_PRELOAD_SO \
    hz6_preload_calloc_direct_target \
    hz6-preload-calloc-direct-target
}

bench_find_hz6_calloc_large_real_target_library() {
  bench_find_hz6_preload_output \
    HZ6_CALLOC_LARGE_REAL_TARGET_PRELOAD_SO \
    hz6_preload_calloc_large_real_target \
    hz6-preload-calloc-large-real-target
}

bench_find_hz6_realloc_boundary_target_library() {
  bench_find_hz6_preload_output \
    HZ6_REALLOC_BOUNDARY_TARGET_PRELOAD_SO \
    hz6_preload_realloc_boundary_target \
    hz6-preload-realloc-boundary-target
}

bench_find_hz6_realloc_boundary_4k_target_library() {
  bench_find_hz6_preload_output \
    HZ6_REALLOC_BOUNDARY_4K_TARGET_PRELOAD_SO \
    hz6_preload_realloc_boundary_4k_target \
    hz6-preload-realloc-boundary-4k-target
}

bench_find_hz6_realloc_boundary_8k_target_library() {
  bench_find_hz6_preload_output \
    HZ6_REALLOC_BOUNDARY_8K_TARGET_PRELOAD_SO \
    hz6_preload_realloc_boundary_8k_target \
    hz6-preload-realloc-boundary-8k-target
}

bench_find_hz6_realloc_boundary_adaptive_target_library() {
  bench_find_hz6_preload_output \
    HZ6_REALLOC_BOUNDARY_ADAPTIVE_TARGET_PRELOAD_SO \
    hz6_preload_realloc_boundary_adaptive_target \
    hz6-preload-realloc-boundary-adaptive-target
}

bench_find_hz6_realloc_boundary_adaptive_4k_target_library() {
  bench_find_hz6_preload_output \
    HZ6_REALLOC_BOUNDARY_ADAPTIVE_4K_TARGET_PRELOAD_SO \
    hz6_preload_realloc_boundary_adaptive_4k_target \
    hz6-preload-realloc-boundary-adaptive-4k-target
}

bench_find_hz6_realloc_boundary_adaptive_8k_target_library() {
  bench_find_hz6_preload_output \
    HZ6_REALLOC_BOUNDARY_ADAPTIVE_8K_TARGET_PRELOAD_SO \
    hz6_preload_realloc_boundary_adaptive_8k_target \
    hz6-preload-realloc-boundary-adaptive-8k-target
}

bench_find_hz6_small_boundary_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_target \
    hz6-preload-small-boundary-target
}

bench_find_hz6_small_boundary_fast_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_FAST_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_fast_target \
    hz6-preload-small-boundary-fast-target
}

bench_find_hz6_small_boundary_trusted_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_TRUSTED_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_trusted_target \
    hz6-preload-small-boundary-trusted-target
}

bench_find_hz6_small_boundary_trusted_toy_map8192_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_trusted_toy_map8192_target \
    hz6-preload-small-boundary-trusted-toy-map8192-target
}

bench_find_hz6_small_boundary_trusted_toy_map8192_external_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_EXTERNAL_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_trusted_toy_map8192_external_target \
    hz6-preload-small-boundary-trusted-toy-map8192-external-target
}

bench_find_hz6_small_boundary_trusted_toy_map8192_external_meta_off_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_EXTERNAL_META_OFF_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_target \
    hz6-preload-small-boundary-trusted-toy-map8192-external-meta-off-target
}

bench_find_hz6_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_EXTERNAL_META_OFF_ROUTE16K_TARGET_PRELOAD_SO \
    hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target \
    hz6-preload-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target
}

bench_find_hz6_toy_map_external_target_library() {
  bench_find_hz6_preload_output \
    HZ6_TOY_MAP_EXTERNAL_TARGET_PRELOAD_SO \
    hz6_preload_toy_map_external_target \
    hz6-preload-toy-map-external-target
}

bench_find_hz6_source_run_meta_off_target_library() {
  bench_find_hz6_preload_output \
    HZ6_SOURCE_RUN_META_OFF_TARGET_PRELOAD_SO \
    hz6_preload_source_run_meta_off_target \
    hz6-preload-source-run-meta-off-target
}

bench_find_hz6_workload_capacity_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_target \
    hz6-preload-workload-capacity-target
}

bench_find_hz6_workload_capacity_lite_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_LITE_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_lite_target \
    hz6-preload-workload-capacity-lite-target
}

bench_find_hz6_workload_capacity_narrow_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_NARROW_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_narrow_target \
    hz6-preload-workload-capacity-narrow-target
}

bench_find_hz6_workload_capacity_hybrid_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_HYBRID_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_hybrid_target \
    hz6-preload-workload-capacity-hybrid-target
}

bench_find_hz6_workload_capacity_lite_map8192_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_LITE_MAP8192_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_lite_map8192_target \
    hz6-preload-workload-capacity-lite-map8192-target
}

bench_find_hz6_workload_capacity_lean_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_LEAN_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_lean_target \
    hz6-preload-workload-capacity-lean-target
}

bench_find_hz6_workload_capacity_plus_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_PLUS_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_plus_target \
    hz6-preload-workload-capacity-plus-target
}

bench_find_hz6_workload_descriptor_overflow_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_DESCRIPTOR_OVERFLOW_TARGET_PRELOAD_SO \
    hz6_preload_workload_descriptor_overflow_target \
    hz6-preload-workload-descriptor-overflow-target
}

bench_find_hz6_workload_descriptor_hybrid_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_DESCRIPTOR_HYBRID_TARGET_PRELOAD_SO \
    hz6_preload_workload_descriptor_hybrid_target \
    hz6-preload-workload-descriptor-hybrid-target
}

bench_find_hz6_workload_capacity_mid_target_library() {
  bench_find_hz6_preload_output \
    HZ6_WORKLOAD_CAPACITY_MID_TARGET_PRELOAD_SO \
    hz6_preload_workload_capacity_mid_target \
    hz6-preload-workload-capacity-mid-target
}

bench_find_hz6_high_remote_owner_inbox_target_library() {
  bench_find_hz6_preload_output \
    HZ6_HIGH_REMOTE_OWNER_INBOX_TARGET_PRELOAD_SO \
    hz6_preload_high_remote_owner_inbox \
    hz6-preload-high-remote-owner-inbox-target
}

bench_find_hz6_high_remote_owner_inbox_direct_reuse_target_library() {
  bench_find_hz6_preload_output \
    HZ6_HIGH_REMOTE_OWNER_INBOX_DIRECT_REUSE_TARGET_PRELOAD_SO \
    hz6_preload_high_remote_owner_inbox_direct_reuse \
    hz6-preload-high-remote-owner-inbox-direct-reuse-target
}

bench_find_hz6_high_remote_transfer_presence_target_library() {
  bench_find_hz6_preload_output \
    HZ6_HIGH_REMOTE_TRANSFER_PRESENCE_TARGET_PRELOAD_SO \
    hz6_preload_high_remote_transfer_presence \
    hz6-preload-high-remote-transfer-presence-target
}

bench_find_hz6_cross128_toy2_split_target_library() {
  bench_find_hz6_preload_output \
    HZ6_CROSS128_TOY2_SPLIT_TARGET_PRELOAD_SO \
    hz6_preload_cross128_toy2_split \
    hz6-preload-cross128-toy2-split-target
}

bench_find_hz6_cross128_toy2_route_before_maps_target_library() {
  bench_find_hz6_preload_output \
    HZ6_CROSS128_TOY2_ROUTE_BEFORE_MAPS_TARGET_PRELOAD_SO \
    hz6_preload_cross128_toy2_route_before_maps \
    hz6-preload-cross128-toy2-route-before-maps-target
}

bench_find_hz6_transfer_class_shard_target_library() {
  bench_find_hz6_preload_output \
    HZ6_TRANSFER_CLASS_SHARD_TARGET_PRELOAD_SO \
    hz6_preload_transfer_class_shard \
    hz6-preload-transfer-class-shard-target
}

bench_find_hz6_transfer_small_class_shard_target_library() {
  bench_find_hz6_preload_output \
    HZ6_TRANSFER_SMALL_CLASS_SHARD_TARGET_PRELOAD_SO \
    hz6_preload_transfer_small_class_shard \
    hz6-preload-transfer-small-class-shard-target
}

bench_find_hz6_midpage_trusted_class_target_library() {
  bench_find_hz6_preload_output \
    HZ6_MIDPAGE_TRUSTED_CLASS_TARGET_PRELOAD_SO \
    hz6_preload_midpage_trusted_class_target \
    hz6-preload-midpage-trusted-class-target
}

bench_find_hz6_midpage_skip_transfer_target_library() {
  bench_find_hz6_preload_output \
    HZ6_MIDPAGE_SKIP_TRANSFER_TARGET_PRELOAD_SO \
    hz6_preload_midpage_skip_transfer_target \
    hz6-preload-midpage-skip-transfer-target
}

bench_find_allocator_library() {
  local allocator="$1"

  case "${allocator}" in
    system)
      printf '%s\n' ""
      ;;
    hz3)
      bench_find_first_existing \
        "${HZ3_SO:-${ROOT_DIR}/libhakozuna_hz3_scale.so}" \
        "${ROOT_DIR}/libhakozuna_hz3_scale.dylib"
      ;;
    hz4)
      bench_find_first_existing \
        "${HZ4_SO:-${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4.so}" \
        "${ROOT_DIR}/hakozuna-mt/libhakozuna_hz4.dylib"
      ;;
    mimalloc)
      bench_find_mimalloc_library
      ;;
    tcmalloc)
      bench_find_tcmalloc_library
      ;;
    hz5)
      bench_find_hz5_library
      ;;
    hz6)
      bench_find_hz6_library
      ;;
    hz8)
      bench_find_hz8_library
      ;;
    hz6-toy-target|hz6_toy_target)
      bench_find_hz6_toy_target_library
      ;;
    hz6-toy-trusted-target|hz6_toy_trusted_target)
      bench_find_hz6_toy_trusted_target_library
      ;;
    hz6-aligned-target|hz6_aligned_target)
      bench_find_hz6_aligned_target_library
      ;;
    hz6-calloc-real-target|hz6_calloc_real_target)
      bench_find_hz6_calloc_real_target_library
      ;;
    hz6-calloc-direct-target|hz6_calloc_direct_target)
      bench_find_hz6_calloc_direct_target_library
      ;;
    hz6-calloc-large-real-target|hz6_calloc_large_real_target)
      bench_find_hz6_calloc_large_real_target_library
      ;;
    hz6-realloc-boundary-target|hz6_realloc_boundary_target)
      bench_find_hz6_realloc_boundary_target_library
      ;;
    hz6-realloc-boundary-4k-target|hz6_realloc_boundary_4k_target)
      bench_find_hz6_realloc_boundary_4k_target_library
      ;;
    hz6-realloc-boundary-8k-target|hz6_realloc_boundary_8k_target)
      bench_find_hz6_realloc_boundary_8k_target_library
      ;;
    hz6-realloc-boundary-adaptive-target|hz6_realloc_boundary_adaptive_target)
      bench_find_hz6_realloc_boundary_adaptive_target_library
      ;;
    hz6-realloc-boundary-adaptive-4k-target|hz6_realloc_boundary_adaptive_4k_target)
      bench_find_hz6_realloc_boundary_adaptive_4k_target_library
      ;;
    hz6-realloc-boundary-adaptive-8k-target|hz6_realloc_boundary_adaptive_8k_target)
      bench_find_hz6_realloc_boundary_adaptive_8k_target_library
      ;;
    hz6-small-boundary-target|hz6_small_boundary_target)
      bench_find_hz6_small_boundary_target_library
      ;;
    hz6-small-boundary-fast-target|hz6_small_boundary_fast_target)
      bench_find_hz6_small_boundary_fast_target_library
      ;;
    hz6-small-boundary-trusted-target|hz6_small_boundary_trusted_target)
      bench_find_hz6_small_boundary_trusted_target_library
      ;;
    hz6-small-boundary-trusted-toy-map8192-target|hz6_small_boundary_trusted_toy_map8192_target)
      bench_find_hz6_small_boundary_trusted_toy_map8192_target_library
      ;;
    hz6-small-boundary-trusted-toy-map8192-external-target|hz6_small_boundary_trusted_toy_map8192_external_target)
      bench_find_hz6_small_boundary_trusted_toy_map8192_external_target_library
      ;;
    hz6-small-boundary-trusted-toy-map8192-external-meta-off-target|hz6_small_boundary_trusted_toy_map8192_external_meta_off_target)
      bench_find_hz6_small_boundary_trusted_toy_map8192_external_meta_off_target_library
      ;;
    hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target|hz6_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target)
      bench_find_hz6_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target_library
      ;;
    hz6-toy-map-external-target|hz6_toy_map_external_target)
      bench_find_hz6_toy_map_external_target_library
      ;;
    hz6-source-run-meta-off-target|hz6_source_run_meta_off_target)
      bench_find_hz6_source_run_meta_off_target_library
      ;;
    hz6-workload-capacity-target|hz6_workload_capacity_target)
      bench_find_hz6_workload_capacity_target_library
      ;;
    hz6-workload-capacity-lite-target|hz6_workload_capacity_lite_target)
      bench_find_hz6_workload_capacity_lite_target_library
      ;;
    hz6-workload-capacity-narrow-target|hz6_workload_capacity_narrow_target)
      bench_find_hz6_workload_capacity_narrow_target_library
      ;;
    hz6-workload-capacity-hybrid-target|hz6_workload_capacity_hybrid_target)
      bench_find_hz6_workload_capacity_hybrid_target_library
      ;;
    hz6-workload-capacity-lite-map8192-target|hz6_workload_capacity_lite_map8192_target)
      bench_find_hz6_workload_capacity_lite_map8192_target_library
      ;;
    hz6-workload-capacity-lean-target|hz6_workload_capacity_lean_target)
      bench_find_hz6_workload_capacity_lean_target_library
      ;;
    hz6-workload-capacity-plus-target|hz6_workload_capacity_plus_target)
      bench_find_hz6_workload_capacity_plus_target_library
      ;;
    hz6-workload-descriptor-overflow-target|hz6_workload_descriptor_overflow_target)
      bench_find_hz6_workload_descriptor_overflow_target_library
      ;;
    hz6-workload-descriptor-hybrid-target|hz6_workload_descriptor_hybrid_target)
      bench_find_hz6_workload_descriptor_hybrid_target_library
      ;;
    hz6-workload-capacity-mid-target|hz6_workload_capacity_mid_target)
      bench_find_hz6_workload_capacity_mid_target_library
      ;;
    hz6-high-remote-owner-inbox-target|hz6_high_remote_owner_inbox_target)
      bench_find_hz6_high_remote_owner_inbox_target_library
      ;;
    hz6-high-remote-owner-inbox-direct-reuse-target|hz6_high_remote_owner_inbox_direct_reuse_target)
      bench_find_hz6_high_remote_owner_inbox_direct_reuse_target_library
      ;;
    hz6-high-remote-transfer-presence-target|hz6_high_remote_transfer_presence_target)
      bench_find_hz6_high_remote_transfer_presence_target_library
      ;;
    hz6-cross128-toy2-split-target|hz6_cross128_toy2_split_target)
      bench_find_hz6_cross128_toy2_split_target_library
      ;;
    hz6-cross128-toy2-route-before-maps-target|hz6_cross128_toy2_route_before_maps_target)
      bench_find_hz6_cross128_toy2_route_before_maps_target_library
      ;;
    hz6-transfer-class-shard-target|hz6_transfer_class_shard_target)
      bench_find_hz6_transfer_class_shard_target_library
      ;;
    hz6-transfer-small-class-shard-target|hz6_transfer_small_class_shard_target)
      bench_find_hz6_transfer_small_class_shard_target_library
      ;;
    hz6-midpage-trusted-class|hz6_midpage_trusted_class)
      bench_find_hz6_midpage_trusted_class_target_library
      ;;
    hz6-midpage-skip-transfer-target|hz6_midpage_skip_transfer_target)
      bench_find_hz6_midpage_skip_transfer_target_library
      ;;
    hakorune-mimalloc|hakorune_mimalloc)
      bench_find_hakorune_mimalloc_library
      ;;
    /*)
      bench_find_first_existing "${allocator}"
      ;;
    *)
      echo "unknown allocator: ${allocator}" >&2
      return 1
      ;;
  esac
}

bench_print_allocator_hints() {
  local allocator="$1"
  case "${allocator}" in
    mimalloc)
      if bench_is_macos; then
        echo "hint: install with 'brew install mimalloc' or set MIMALLOC_SO" >&2
      else
        echo "hint: install the distro mimalloc package, run './linux/prepare_linux_bench_allocators.sh', or set MIMALLOC_SO" >&2
      fi
      ;;
    tcmalloc)
      if bench_is_macos; then
        echo "hint: install with 'brew install gperftools' or set TCMALLOC_SO" >&2
      else
        echo "hint: install the distro gperftools / tcmalloc package, run './linux/prepare_linux_bench_allocators.sh', or set TCMALLOC_SO" >&2
      fi
      ;;
    hakorune-mimalloc|hakorune_mimalloc)
      echo "hint: extract hakorune-mimalloc-provider.zip under private/bench-assets/linux/allocators or set HAKORUNE_MIMALLOC_PROVIDER_DIR" >&2
      ;;
    hz5)
      echo "hint: build the HZ5 preload full lane with './linux/build_linux_hz5_preload_full.sh' or set HZ5_PRELOAD_FULL_SO" >&2
      ;;
    hz6)
      echo "hint: build the HZ6 preload lane with './hakozuna-hz6/linux/build_hz6_preload.sh' or set HZ6_PRELOAD_SO" >&2
      ;;
    hz8)
      echo "hint: build HZ8 preload with 'make -C hakozuna-hz8 preload-smoke' or set HZ8_SO" >&2
      ;;
    hz6-toy-target|hz6_toy_target)
      echo "hint: build the HZ6 Toy target lane with './hakozuna-hz6/linux/build_hz6_preload_toy_target.sh' or set HZ6_TOY_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-toy-trusted-target|hz6_toy_trusted_target)
      echo "hint: build the HZ6 Toy trusted target lane with './hakozuna-hz6/linux/build_hz6_preload_toy_trusted_target.sh' or set HZ6_TOY_TRUSTED_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-aligned-target|hz6_aligned_target)
      echo "hint: build the HZ6 aligned target lane with './hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh' or set HZ6_ALIGNED_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-calloc-real-target|hz6_calloc_real_target)
      echo "hint: build the HZ6 calloc real target lane with './hakozuna-hz6/linux/build_hz6_preload_calloc_real_target.sh' or set HZ6_CALLOC_REAL_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-calloc-direct-target|hz6_calloc_direct_target)
      echo "hint: build the HZ6 calloc direct target lane with './hakozuna-hz6/linux/build_hz6_preload_calloc_direct_target.sh' or set HZ6_CALLOC_DIRECT_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-calloc-large-real-target|hz6_calloc_large_real_target)
      echo "hint: build the HZ6 calloc large-real target lane with './hakozuna-hz6/linux/build_hz6_preload_calloc_large_real_target.sh' or set HZ6_CALLOC_LARGE_REAL_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-realloc-boundary-target|hz6_realloc_boundary_target)
      echo "hint: build the HZ6 realloc-boundary target lane with './hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh' or set HZ6_REALLOC_BOUNDARY_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-realloc-boundary-4k-target|hz6_realloc_boundary_4k_target)
      echo "hint: build the HZ6 realloc-boundary 4K target lane with './hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_4k_target.sh' or set HZ6_REALLOC_BOUNDARY_4K_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-realloc-boundary-8k-target|hz6_realloc_boundary_8k_target)
      echo "hint: build the HZ6 realloc-boundary 8K target lane with './hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_8k_target.sh' or set HZ6_REALLOC_BOUNDARY_8K_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-realloc-boundary-adaptive-target|hz6_realloc_boundary_adaptive_target)
      echo "hint: build the HZ6 realloc-boundary adaptive target lane with './hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_adaptive_target.sh' or set HZ6_REALLOC_BOUNDARY_ADAPTIVE_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-realloc-boundary-adaptive-4k-target|hz6_realloc_boundary_adaptive_4k_target)
      echo "hint: build the HZ6 realloc-boundary adaptive 4K target lane with './hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_adaptive_4k_target.sh' or set HZ6_REALLOC_BOUNDARY_ADAPTIVE_4K_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-realloc-boundary-adaptive-8k-target|hz6_realloc_boundary_adaptive_8k_target)
      echo "hint: build the HZ6 realloc-boundary adaptive 8K target lane with './hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_adaptive_8k_target.sh' or set HZ6_REALLOC_BOUNDARY_ADAPTIVE_8K_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-target|hz6_small_boundary_target)
      echo "hint: build the HZ6 small-boundary target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh' or set HZ6_SMALL_BOUNDARY_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-fast-target|hz6_small_boundary_fast_target)
      echo "hint: build the HZ6 small-boundary fast target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh' or set HZ6_SMALL_BOUNDARY_FAST_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-trusted-target|hz6_small_boundary_trusted_target)
      echo "hint: build the HZ6 small-boundary trusted target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_target.sh' or set HZ6_SMALL_BOUNDARY_TRUSTED_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-trusted-toy-map8192-target|hz6_small_boundary_trusted_toy_map8192_target)
      echo "hint: build the HZ6 small-boundary trusted Toy-map8192 target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_target.sh' or set HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-trusted-toy-map8192-external-target|hz6_small_boundary_trusted_toy_map8192_external_target)
      echo "hint: build the HZ6 small-boundary trusted Toy-map8192 external target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_target.sh' or set HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_EXTERNAL_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-trusted-toy-map8192-external-meta-off-target|hz6_small_boundary_trusted_toy_map8192_external_meta_off_target)
      echo "hint: build the HZ6 small-boundary trusted Toy-map8192 external meta-off target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_target.sh' or set HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_EXTERNAL_META_OFF_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target|hz6_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target)
      echo "hint: build the HZ6 small-boundary trusted Toy-map8192 external meta-off route16k target lane with './hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target.sh' or set HZ6_SMALL_BOUNDARY_TRUSTED_TOY_MAP8192_EXTERNAL_META_OFF_ROUTE16K_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-toy-map-external-target|hz6_toy_map_external_target)
      echo "hint: build the HZ6 Toy map external target lane with './hakozuna-hz6/linux/build_hz6_preload_toy_map_external_target.sh' or set HZ6_TOY_MAP_EXTERNAL_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-source-run-meta-off-target|hz6_source_run_meta_off_target)
      echo "hint: build the HZ6 source-run meta-off target lane with './hakozuna-hz6/linux/build_hz6_preload_source_run_meta_off_target.sh' or set HZ6_SOURCE_RUN_META_OFF_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-target|hz6_workload_capacity_target)
      echo "hint: build the HZ6 workload-capacity target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_target.sh' or set HZ6_WORKLOAD_CAPACITY_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-lite-target|hz6_workload_capacity_lite_target)
      echo "hint: build the HZ6 workload-capacity lite target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lite_target.sh' or set HZ6_WORKLOAD_CAPACITY_LITE_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-narrow-target|hz6_workload_capacity_narrow_target)
      echo "hint: build the HZ6 workload-capacity narrow target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_narrow_target.sh' or set HZ6_WORKLOAD_CAPACITY_NARROW_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-hybrid-target|hz6_workload_capacity_hybrid_target)
      echo "hint: build the HZ6 workload-capacity hybrid target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_hybrid_target.sh' or set HZ6_WORKLOAD_CAPACITY_HYBRID_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-lite-map8192-target|hz6_workload_capacity_lite_map8192_target)
      echo "hint: build the HZ6 workload-capacity lite-map8192 target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lite_map8192_target.sh' or set HZ6_WORKLOAD_CAPACITY_LITE_MAP8192_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-lean-target|hz6_workload_capacity_lean_target)
      echo "hint: build the HZ6 workload-capacity lean target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lean_target.sh' or set HZ6_WORKLOAD_CAPACITY_LEAN_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-plus-target|hz6_workload_capacity_plus_target)
      echo "hint: build the HZ6 workload-capacity plus target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_plus_target.sh' or set HZ6_WORKLOAD_CAPACITY_PLUS_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-descriptor-overflow-target|hz6_workload_descriptor_overflow_target)
      echo "hint: build the HZ6 workload descriptor-overflow target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_descriptor_overflow_target.sh' or set HZ6_WORKLOAD_DESCRIPTOR_OVERFLOW_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-descriptor-hybrid-target|hz6_workload_descriptor_hybrid_target)
      echo "hint: build the HZ6 workload descriptor-hybrid target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_descriptor_hybrid_target.sh' or set HZ6_WORKLOAD_DESCRIPTOR_HYBRID_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-workload-capacity-mid-target|hz6_workload_capacity_mid_target)
      echo "hint: build the HZ6 workload-capacity mid target lane with './hakozuna-hz6/linux/build_hz6_preload_workload_capacity_mid_target.sh' or set HZ6_WORKLOAD_CAPACITY_MID_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-high-remote-owner-inbox-target|hz6_high_remote_owner_inbox_target)
      echo "hint: build the HZ6 high-remote owner-inbox target lane with './hakozuna-hz6/linux/build_hz6_preload_high_remote_owner_inbox_target.sh' or set HZ6_HIGH_REMOTE_OWNER_INBOX_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-high-remote-owner-inbox-direct-reuse-target|hz6_high_remote_owner_inbox_direct_reuse_target)
      echo "hint: build the HZ6 high-remote owner-inbox DirectReuse target lane with './hakozuna-hz6/linux/build_hz6_preload_high_remote_owner_inbox_direct_reuse_target.sh' or set HZ6_HIGH_REMOTE_OWNER_INBOX_DIRECT_REUSE_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-high-remote-transfer-presence-target|hz6_high_remote_transfer_presence_target)
      echo "hint: build the HZ6 high-remote transfer-presence target lane with './hakozuna-hz6/linux/build_hz6_preload_high_remote_transfer_presence_target.sh' or set HZ6_HIGH_REMOTE_TRANSFER_PRESENCE_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-transfer-class-shard-target|hz6_transfer_class_shard_target)
      echo "hint: build the HZ6 transfer class-shard target lane with './hakozuna-hz6/linux/build_hz6_preload_transfer_class_shard_target.sh' or set HZ6_TRANSFER_CLASS_SHARD_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-transfer-small-class-shard-target|hz6_transfer_small_class_shard_target)
      echo "hint: build the HZ6 transfer small-class-shard target lane with './hakozuna-hz6/linux/build_hz6_preload_transfer_small_class_shard_target.sh' or set HZ6_TRANSFER_SMALL_CLASS_SHARD_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-midpage-trusted-class|hz6_midpage_trusted_class)
      echo "hint: build the HZ6 MidPage trusted-class target lane with './hakozuna-hz6/linux/build_hz6_preload_midpage_trusted_class_target.sh' or set HZ6_MIDPAGE_TRUSTED_CLASS_TARGET_PRELOAD_SO" >&2
      ;;
    hz6-midpage-skip-transfer-target|hz6_midpage_skip_transfer_target)
      echo "hint: build the HZ6 MidPage skip-transfer target lane with './hakozuna-hz6/linux/build_hz6_preload_midpage_skip_transfer_target.sh' or set HZ6_MIDPAGE_SKIP_TRANSFER_TARGET_PRELOAD_SO" >&2
      ;;
    hz3)
      echo "hint: build hz3 first, for example './linux/build_linux_release_lane.sh' or './mac/build_mac_release_lane.sh'" >&2
      ;;
    hz4)
      echo "hint: build hz4 first, for example './linux/build_linux_release_lane.sh' or './mac/build_mac_release_lane.sh'" >&2
      ;;
  esac
}

bench_run_with_allocator() {
  local allocator="$1"
  local lib_path="$2"
  shift 2

  if [[ "${allocator}" == "system" ]]; then
    "$@"
    return $?
  fi

  if bench_is_macos; then
    DYLD_INSERT_LIBRARIES="${lib_path}" "$@"
  else
    case "${allocator}" in
      hakorune-mimalloc|hakorune_mimalloc)
        local provider_dir
        provider_dir="$(cd "$(dirname "${lib_path}")" && pwd)"
        env \
          HAKORUNE_PROVIDER_LIBRARY="${HAKORUNE_PROVIDER_LIBRARY:-${provider_dir}/libhakorune_provider.so}" \
          HAKORUNE_PROVIDER_LDPRELOAD_REPORT="${HAKORUNE_PROVIDER_LDPRELOAD_REPORT:-${provider_dir}/ldpreload_counts.out}" \
          LD_PRELOAD="${lib_path}" \
          "$@"
        ;;
      *)
        env LD_PRELOAD="${lib_path}" "$@"
        ;;
    esac
  fi
}
