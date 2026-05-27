#include "hz6_transfer_backend.h"

int hz6_transfer_backend_pop_from_shard(Hz6TransferBackend* backend,
                                        uint16_t class_id,
                                        size_t home_shard,
                                        Hz6TransferObject* out) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return hz6_transfer_pop(&backend->single_cache, class_id, out);
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE ||
      backend->shard_count == 0) {
    return 0;
  }

  size_t start = home_shard % backend->shard_count;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    size_t shard_index = (start + i) % backend->shard_count;
    if (hz6_transfer_pop(&backend->shard[shard_index], class_id, out)) {
      return 1;
    }
  }
  return 0;
}
