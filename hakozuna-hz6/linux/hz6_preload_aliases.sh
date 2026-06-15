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
      "hz6-midpage-trusted-class" "hz6_midpage_trusted_class"; then
    "${root_dir}/hakozuna-hz6/linux/build_hz6_preload_midpage_trusted_class_target.sh"
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
}
