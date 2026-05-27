#include "hz6_transfer_backend.h"

size_t hz6_transfer_backend_shard_count_at(const Hz6TransferBackend* backend,
                                           size_t shard_index) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return shard_index == 0 ? hz6_transfer_count(&backend->single_cache) : 0;
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE ||
      shard_index >= backend->shard_count) {
    return 0;
  }
  return hz6_transfer_count(&backend->shard[shard_index]);
}

size_t hz6_transfer_backend_shard_capacity_at(
    const Hz6TransferBackend* backend,
    size_t shard_index) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return shard_index == 0 ? backend->single_cache.capacity : 0;
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE ||
      shard_index >= backend->shard_count) {
    return 0;
  }
  return backend->shard[shard_index].capacity;
}
