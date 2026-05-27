#include "hz6_transfer_backend.h"

void hz6_transfer_backend_init_single(Hz6TransferBackend* backend,
                                      Hz6TransferObject* objects,
                                      size_t capacity) {
  if (!backend) {
    return;
  }
  backend->kind = HZ6_TRANSFER_BACKEND_SINGLE_CACHE;
  backend->shard_count = 0;
  backend->next_push_shard = 0;
  hz6_transfer_init(&backend->single_cache, objects, capacity);
}

int hz6_transfer_backend_push(Hz6TransferBackend* backend,
                              Hz6TransferObject object) {
  if (!backend) {
    return 0;
  }
  return hz6_transfer_backend_push_to_shard(backend, object,
                                            backend->next_push_shard);
}

int hz6_transfer_backend_pop(Hz6TransferBackend* backend,
                             uint16_t class_id,
                             Hz6TransferObject* out) {
  return hz6_transfer_backend_pop_from_shard(backend, class_id,
                                             (size_t)class_id, out);
}
