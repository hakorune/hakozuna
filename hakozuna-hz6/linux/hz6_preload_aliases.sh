#!/usr/bin/env bash

hz6_preload_allocator_requested() {
  local allocators_csv="$1"
  local alias_dash="$2"
  local alias_underscore="$3"
  [[ ",${allocators_csv}," == *",${alias_dash},"* ||
     ",${allocators_csv}," == *",${alias_underscore},"* ]]
}

hz6_preload_build_requested_aliases() {
  local allocators_csv="$1"
  local root_dir="$2"

  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-toy-target" "hz6_toy_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_toy_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-toy-trusted-target" "hz6_toy_trusted_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_toy_trusted_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-aligned-target" "hz6_aligned_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_aligned_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-calloc-real-target" "hz6_calloc_real_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_calloc_real_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-calloc-direct-target" "hz6_calloc_direct_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_calloc_direct_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-calloc-large-real-target" "hz6_calloc_large_real_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_calloc_large_real_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-target" "hz6_small_boundary_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-fast-target" "hz6_small_boundary_fast_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_fast_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-trusted-target" "hz6_small_boundary_trusted_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-trusted-toy-map8192-target" \
      "hz6_small_boundary_trusted_toy_map8192_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-trusted-toy-map8192-external-target" \
      "hz6_small_boundary_trusted_toy_map8192_external_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-trusted-toy-map8192-external-meta-off-target" \
      "hz6_small_boundary_trusted_toy_map8192_external_meta_off_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-small-boundary-trusted-toy-map8192-external-meta-off-route16k-target" \
      "hz6_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_toy_map8192_external_meta_off_route16k_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-toy-map-external-target" \
      "hz6_toy_map_external_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_toy_map_external_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-source-run-meta-off-target" \
      "hz6_source_run_meta_off_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_source_run_meta_off_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-target" \
      "hz6_workload_capacity_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-lite-target" \
      "hz6_workload_capacity_lite_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lite_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-narrow-target" \
      "hz6_workload_capacity_narrow_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_narrow_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-hybrid-target" \
      "hz6_workload_capacity_hybrid_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_hybrid_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-lite-map8192-target" \
      "hz6_workload_capacity_lite_map8192_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lite_map8192_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-lean-target" \
      "hz6_workload_capacity_lean_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_lean_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-plus-target" \
      "hz6_workload_capacity_plus_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_plus_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-descriptor-overflow-target" \
      "hz6_workload_descriptor_overflow_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_descriptor_overflow_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-descriptor-hybrid-target" \
      "hz6_workload_descriptor_hybrid_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_descriptor_hybrid_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-workload-capacity-mid-target" \
      "hz6_workload_capacity_mid_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_workload_capacity_mid_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-high-remote-owner-inbox-target" \
      "hz6_high_remote_owner_inbox_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_high_remote_owner_inbox_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-transfer-class-shard-target" \
      "hz6_transfer_class_shard_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_transfer_class_shard_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-midpage-trusted-class" "hz6_midpage_trusted_class"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_midpage_trusted_class_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-midpage-skip-transfer-target" \
      "hz6_midpage_skip_transfer_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_midpage_skip_transfer_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-realloc-boundary-target" "hz6_realloc_boundary_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-realloc-boundary-4k-target" "hz6_realloc_boundary_4k_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_4k_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-realloc-boundary-8k-target" "hz6_realloc_boundary_8k_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_8k_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-realloc-boundary-adaptive-target" \
      "hz6_realloc_boundary_adaptive_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_adaptive_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-realloc-boundary-adaptive-4k-target" \
      "hz6_realloc_boundary_adaptive_4k_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_adaptive_4k_target.sh"
  fi
  if hz6_preload_allocator_requested "$allocators_csv" \
      "hz6-realloc-boundary-adaptive-8k-target" \
      "hz6_realloc_boundary_adaptive_8k_target"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_realloc_boundary_adaptive_8k_target.sh"
  fi
}
