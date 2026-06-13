#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/hakozuna-hz6/out/linux/hz6_preload}"
CC_BIN="${CC:-cc}"
HZ6_DIR="${ROOT_DIR}/hakozuna-hz6"
HZ6_EXTRA_CFLAGS="${HZ6_EXTRA_CFLAGS:-}"
HZ6_PRELOAD_DEFAULT_CFLAGS="${HZ6_PRELOAD_DEFAULT_CFLAGS:--DHZ6_ROUTE_TABLE_CAPACITY=131072 -DHZ6_OBJECT_DESCRIPTOR_CAPACITY=32768 -DHZ6_SOURCE_BLOCK_CAPACITY=4096 -DHZ6_FRONT_CACHE_BIN_CAPACITY=8192 -DHZ6_TOY_SMALL_ACTIVE_FREE_MAP_CAPACITY=32768 -DHZ6_LINUX_MMAP_RETAIN_L1=1 -DHZ6_LINUX_MMAP_RETAIN_64K_STACK_L1=1 -DHZ6_TOY_FULL_BLOCK_PREFILL_L1=1 -DHZ6_TOY_FULL_BLOCK_PREFILL_MAX_SLOTS=128 -DHZ6_ROUTE_TOMBSTONE_COMPACT_L1=1 -DHZ6_ROUTE_HASH_XOR_FOLD_L1=1 -DHZ6_ROUTE_LINEAR_WRAP_L1=1 -DHZ6_ROUTE_LOOP_CARRY_L1=1}"

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
  "${HZ6_DIR}/preload"
)

HZ6_LIB_SOURCES=(
  "${HZ6_DIR}/api/hz6_allocator.c"
  "${HZ6_DIR}/api/hz6_allocator_descriptor.c"
  "${HZ6_DIR}/api/hz6_allocator_frontcache_mutation.c"
  "${HZ6_DIR}/api/hz6_allocator_frontcache_query.c"
  "${HZ6_DIR}/api/hz6_allocator_malloc.c"
  "${HZ6_DIR}/api/hz6_allocator_free.c"
  "${HZ6_DIR}/api/hz6_allocator_free_remote.c"
  "${HZ6_DIR}/api/hz6_allocator_prefill.c"
  "${HZ6_DIR}/api/hz6_allocator_profile_query.c"
  "${HZ6_DIR}/api/hz6_allocator_profile_source.c"
  "${HZ6_DIR}/api/hz6_allocator_large_span_stats.c"
  "${HZ6_DIR}/api/hz6_allocator_owns.c"
  "${HZ6_DIR}/api/hz6_allocator_stats_snapshot.c"
  "${HZ6_DIR}/api/hz6_allocator_init.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state_owner.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state_source_blocks.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state_descriptors.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state_frontcache.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state_large_span_pool.c"
  "${HZ6_DIR}/api/hz6_allocator_init_state.c"
  "${HZ6_DIR}/api/hz6_allocator_init_backends.c"
  "${HZ6_DIR}/api/hz6_allocator_destroy_descriptors.c"
  "${HZ6_DIR}/api/hz6_allocator_destroy_source_blocks.c"
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
  "${HZ6_DIR}/api/hz6_allocator_scavenge_orphans.c"
  "${HZ6_DIR}/api/hz6_allocator_scavenge_local_free.c"
  "${HZ6_DIR}/api/hz6_allocator_scavenge_profile.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_create.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_lifetime.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_route.c"
  "${HZ6_DIR}/api/hz6_allocator_source_block_route_dryrun.c"
  "${HZ6_DIR}/api/hz6_allocator_transfer_query.c"
  "${HZ6_DIR}/api/hz6_allocator_transfer_dispatch.c"
  "${HZ6_DIR}/api/hz6_allocator_large_span_pool.c"
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
  "${HZ6_DIR}/fronts/large/hz6_large128_front_central.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front_free.c"
  "${HZ6_DIR}/fronts/large/hz6_large128_front_ops.c"
  "${HZ6_DIR}/fronts/large/hz6_large_span_class.c"
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
  "${HZ6_DIR}/route/hz6_route_backend_compact.c"
  "${HZ6_DIR}/route/hz6_route_backend_register.c"
  "${HZ6_DIR}/route/hz6_route_backend_unregister.c"
  "${HZ6_DIR}/route/hz6_route_backend_lookup.c"
  "${HZ6_DIR}/route/hz6_route_backend_page_table.c"
  "${HZ6_DIR}/route/hz6_route_backend_page_table_exact.c"
  "${HZ6_DIR}/route/hz6_route_backend_page_table_invalid.c"
  "${HZ6_DIR}/route/hz6_route_table_core.c"
  "${HZ6_DIR}/route/hz6_route_table_compact.c"
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
  "${HZ6_DIR}/transfer/hz6_transfer_backend_sharded_push.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_sharded_pop.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_stats_aggregate.c"
  "${HZ6_DIR}/transfer/hz6_transfer_backend_stats_shards.c"
  "${HZ6_DIR}/transfer/hz6_transfer.c"
)

PRELOAD_SOURCE="${HZ6_DIR}/preload/hz6_preload.c"
PRELOAD_SO="${OUT_DIR}/libhakozuna_hz6_preload.so"

HZ6_INCLUDE_FLAGS=()
for include_dir in "${HZ6_INCLUDES[@]}"; do
  HZ6_INCLUDE_FLAGS+=("-I${include_dir}")
done

HZ6_EXTRA_CFLAGS_ARRAY=()
if [[ -n "${HZ6_PRELOAD_DEFAULT_CFLAGS}" ]]; then
  # shellcheck disable=SC2206
  HZ6_EXTRA_CFLAGS_ARRAY=(${HZ6_PRELOAD_DEFAULT_CFLAGS})
fi
if [[ -n "${HZ6_EXTRA_CFLAGS}" ]]; then
  # shellcheck disable=SC2206
  HZ6_EXTRA_CFLAGS_ARRAY+=(${HZ6_EXTRA_CFLAGS})
fi

command -v "$CC_BIN" >/dev/null 2>&1 || {
  echo "compiler not found in PATH: $CC_BIN" >&2
  exit 1
}

echo "[linux][hz6] building preload: ${PRELOAD_SO}"
"$CC_BIN" -std=c11 -Wall -Wextra -Werror -O2 -fPIC -shared \
  -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -ftls-model=initial-exec \
  -fno-builtin \
  "${HZ6_EXTRA_CFLAGS_ARRAY[@]}" \
  "${HZ6_INCLUDE_FLAGS[@]}" \
  "${HZ6_LIB_SOURCES[@]}" \
  "$PRELOAD_SOURCE" \
  -ldl -pthread \
  -o "$PRELOAD_SO"
echo "[linux][hz6] preload output: ${PRELOAD_SO}"
