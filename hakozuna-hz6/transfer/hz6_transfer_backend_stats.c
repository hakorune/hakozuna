#include "hz6_transfer_backend.h"

size_t hz6_transfer_backend_count(const Hz6TransferBackend* backend) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return hz6_transfer_count(&backend->single_cache);
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE) {
    return 0;
  }
  size_t count = 0;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    count += hz6_transfer_count(&backend->shard[i]);
  }
  return count;
}

size_t hz6_transfer_backend_capacity(const Hz6TransferBackend* backend) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return backend->single_cache.capacity;
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE) {
    return 0;
  }
  size_t capacity = 0;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    capacity += backend->shard[i].capacity;
  }
  return capacity;
}

size_t hz6_transfer_backend_count_class(const Hz6TransferBackend* backend,
                                        uint16_t class_id) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return hz6_transfer_count_class(&backend->single_cache, class_id);
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE) {
    return 0;
  }
  size_t count = 0;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    count += hz6_transfer_count_class(&backend->shard[i], class_id);
  }
  return count;
}

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
