#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${ROOT_DIR}/hakozuna-hz6/out/linux"
CC_BIN="${CC:-cc}"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"

mkdir -p "$OUT_DIR"

HZ6_INCLUDES=(
  "${HZ6_DIR}/include"
  "${HZ6_DIR}/route"
  "${HZ6_DIR}/scavenge"
  "${HZ6_DIR}/transfer"
  "${HZ6_DIR}/owner"
  "${HZ6_DIR}/source"
  "${HZ6_DIR}/frontcache"
  "${HZ6_DIR}/fronts"
  "${HZ6_DIR}/fronts/large"
  "${HZ6_DIR}/fronts/local2p"
  "${HZ6_DIR}/fronts/midpage"
  "${HZ6_DIR}/fronts/toy"
  "${HZ6_DIR}/policy"
  "${HZ6_DIR}/api"
)

HZ6_LIB_SOURCES=(
  "${HZ6_DIR}/api/hz6_allocator.c"
  "${HZ6_DIR}/api/hz6_allocator_descriptor.c"
  "${HZ6_DIR}/api/hz6_allocator_frontcache.c"
  "${HZ6_DIR}/api/hz6_allocator_malloc.c"
  "${HZ6_DIR}/api/hz6_allocator_free.c"
  "${HZ6_DIR}/api/hz6_allocator_free_remote.c"
  "${HZ6_DIR}/api/hz6_allocator_prefill.c"
  "${HZ6_DIR}/api/hz6_allocator_profile.c"
  "${HZ6_DIR}/api/hz6_allocator_init.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state.c"
  "${HZ6_DIR}/api/hz6_allocator_init_backends.c"
  "${HZ6_DIR}/api/hz6_allocator_destroy.c"
  "${HZ6_DIR}/api/hz6_allocator_descriptor_local_cache.c"
  "${HZ6_DIR}/api/hz6_allocator_descriptor_remote_transfer.c"
  "${HZ6_DIR}/api/hz6_allocator_descriptor_prepare.c"
  "${HZ6_DIR}/api/hz6_allocator_descriptor_release.c"
  "${HZ6_DIR}/api/hz6_allocator_owner_dead.c"
  "${HZ6_DIR}/api/hz6_allocator_orphan_release.c"
  "${HZ6_DIR}/api/hz6_allocator_orphan_adopt_prepare.c"
  "${HZ6_DIR}/api/hz6_allocator_orphan_adopt_commit.c"
  "${HZ6_DIR}/api/hz6_allocator_orphan_adopt.c"
  "${HZ6_DIR}/api/hz6_allocator_remote_pending.c"
  "${HZ6_DIR}/api/hz6_allocator_route.c"
  "${HZ6_DIR}/api/hz6_allocator_query.c"
  "${HZ6_DIR}/api/hz6_allocator_scavenge_orphans.c"
  "${HZ6_DIR}/api/hz6_allocator_scavenge_local_free.c"
  "${HZ6_DIR}/api/hz6_allocator_scavenge_profile.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_create.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_lifetime.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_route.c"
  "${HZ6_DIR}/api/hz6_allocator_transfer_query.c"
  "${HZ6_DIR}/api/hz6_allocator_transfer_dispatch.c"
  "${HZ6_DIR}/frontcache/hz6_frontcache.c"
  "${HZ6_DIR}/frontcache/hz6_size_class.c"
  "${HZ6_DIR}/fronts/hz6_front_registry.c"
  "${HZ6_DIR}/fronts/hz6_front_prefill_dispatch.c"
  "${HZ6_DIR}/fronts/hz6_front_source_kind.c"
  "${HZ6_DIR}/fronts/hz6_front_source_ops.c"
  "${HZ6_DIR}/fronts/hz6_front_source_block.c"
  "${HZ6_DIR}/fronts/hz6_front_source_prefill_one.c"
  "${HZ6_DIR}/fronts/hz6_front_source_prefill.c"
  "${HZ6_DIR}/fronts/hz6_front_reuse_transfer.c"
  "${HZ6_DIR}/fronts/hz6_front_source_slot_kind.c"
  "${HZ6_DIR}/fronts/hz6_front_source_slot_ops.c"
  "${HZ6_DIR}/fronts/hz6_front_reuse.c"
  "${HZ6_DIR}/fronts/hz6_front_free.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_prefill.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_prefill_policy.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_policy.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front_alloc.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front_free.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front_ops.c"
  "${HZ6_DIR}/fronts/local2p/hz6_local2p_front_alloc.c"
  "${HZ6_DIR}/fronts/local2p/hz6_local2p_front_free.c"
  "${HZ6_DIR}/fronts/local2p/hz6_local2p_front_prefill.c"
  "${HZ6_DIR}/fronts/local2p/hz6_local2p_front_ops.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_front_alloc.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_front_free.c"
  "${HZ6_DIR}/fronts/midpage/hz6_midpage_front_ops.c"
  "${HZ6_DIR}/fronts/toy/hz6_toy_front.c"
  "${HZ6_DIR}/owner/hz6_owner.c"
  "${HZ6_DIR}/policy/hz6_profiles_config.c"
  "${HZ6_DIR}/policy/hz6_profiles_policy.c"
  "${HZ6_DIR}/route/hz6_route_backend_init.c"
  "${HZ6_DIR}/route/hz6_route_backend_register.c"
  "${HZ6_DIR}/route/hz6_route_backend_unregister.c"
  "${HZ6_DIR}/route/hz6_route_backend_lookup.c"
  "${HZ6_DIR}/route/hz6_route_backend_page_table.c"
  "${HZ6_DIR}/route/hz6_route_table_core.c"
  "${HZ6_DIR}/route/hz6_route_table_exact.c"
  "${HZ6_DIR}/route/hz6_route_table_invalid.c"
  "${HZ6_DIR}/route/hz6_route.c"
  "${HZ6_DIR}/scavenge/hz6_scavenge.c"
  "${HZ6_DIR}/source/linux_source_mmap_ops.c"
  "${HZ6_DIR}/source/linux_source_mmap_memory.c"
  "${HZ6_DIR}/source/hz6_source_system_ops.c"
  "${HZ6_DIR}/source/hz6_source_system_memory.c"
  "${HZ6_DIR}/source/hz6_source_registry_init.c"
  "${HZ6_DIR}/source/hz6_source_registry_lookup.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_sharded_init.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_sharded_ops.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_stats_aggregate.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_stats_shards.c"
  "${HZ6_DIR}/transfer/hz6_transfer.c"
)

HZ6_SMOKES=(
  "tests/hz6_r1_core_contract_smoke.c:hz6_r1_core_contract_smoke"
  "tests/hz6_r1_route_smoke.c:hz6_r1_route_smoke"
  "tests/hz6_r1_transfer_contract_smoke.c:hz6_r1_transfer_contract_smoke"
  "tests/hz6_r1_source_contract_smoke.c:hz6_r1_source_contract_smoke"
  "tests/hz6_r1_allocator_smoke.c:hz6_r1_allocator_smoke"
  "tests/hz6_r1_prefill_smoke.c:hz6_r1_prefill_smoke"
  "tests/hz6_r1_sourceblock_smoke.c:hz6_r1_sourceblock_smoke"
  "tests/hz6_r1_transfer_smoke.c:hz6_r1_transfer_smoke"
  "tests/hz6_r1_reclaim_smoke.c:hz6_r1_reclaim_smoke"
  "tests/hz6_r1_safety_smoke.c:hz6_r1_safety_smoke"
)

HZ6_INCLUDE_FLAGS=()
for include_dir in "${HZ6_INCLUDES[@]}"; do
  HZ6_INCLUDE_FLAGS+=("-I${include_dir}")
done

build_smoke() {
  local test_source="$1"
  local output_name="$2"

  "$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 \
    "${HZ6_INCLUDE_FLAGS[@]}" \
    "${HZ6_LIB_SOURCES[@]}" \
    "$test_source" \
    -o "${OUT_DIR}/${output_name}"

  "${OUT_DIR}/${output_name}"
}

for smoke in "${HZ6_SMOKES[@]}"; do
  test_source="${smoke%%:*}"
  output_name="${smoke##*:}"
  build_smoke "${HZ6_DIR}/${test_source}" "$output_name"
done
