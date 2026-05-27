#include "hz6_transfer_backend.h"

void hz6_transfer_backend_init_sharded(Hz6TransferBackend* backend,
                                       Hz6TransferObject* objects,
                                       size_t capacity,
                                       size_t shard_count) {
  if (!backend) {
    return;
  }
  if (!objects || capacity == 0 || shard_count < 2) {
    hz6_transfer_backend_init_single(backend, objects, capacity);
    return;
  }
  if (shard_count > HZ6_TRANSFER_SHARD_COUNT) {
    shard_count = HZ6_TRANSFER_SHARD_COUNT;
  }
  if (capacity < shard_count) {
    hz6_transfer_backend_init_single(backend, objects, capacity);
    return;
  }

  backend->kind = HZ6_TRANSFER_BACKEND_SHARDED_CACHE;
  backend->single_cache.objects = NULL;
  backend->single_cache.capacity = 0;
  backend->single_cache.count = 0;
  backend->shard_count = shard_count;
  backend->next_push_shard = 0;

  size_t base_capacity = capacity / shard_count;
  size_t remainder = capacity % shard_count;
  size_t offset = 0;
  for (size_t i = 0; i < shard_count; ++i) {
    size_t shard_capacity = base_capacity + (i < remainder ? 1u : 0u);
    hz6_transfer_init(&backend->shard[i], objects + offset, shard_capacity);
    offset += shard_capacity;
  }
  for (size_t i = shard_count; i < HZ6_TRANSFER_SHARD_COUNT; ++i) {
    hz6_transfer_init(&backend->shard[i], NULL, 0);
  }
}
