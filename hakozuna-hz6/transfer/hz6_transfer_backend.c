#include "hz6_transfer_backend.h"

void hz6_transfer_backend_init_single(Hz6TransferBackend* backend,
                                      Hz6TransferObject* objects,
                                      size_t capacity) {
  if (!backend) {
    return;
  }
  backend->kind = HZ6_TRANSFER_BACKEND_SINGLE_CACHE;
  hz6_transfer_init(&backend->single_cache, objects, capacity);
}

int hz6_transfer_backend_push(Hz6TransferBackend* backend,
                              Hz6TransferObject object) {
  if (!backend || backend->kind != HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return 0;
  }
  return hz6_transfer_push(&backend->single_cache, object);
}

int hz6_transfer_backend_pop(Hz6TransferBackend* backend,
                             uint16_t class_id,
                             Hz6TransferObject* out) {
  if (!backend || backend->kind != HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return 0;
  }
  return hz6_transfer_pop(&backend->single_cache, class_id, out);
}

size_t hz6_transfer_backend_count(const Hz6TransferBackend* backend) {
  if (!backend || backend->kind != HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return 0;
  }
  return hz6_transfer_count(&backend->single_cache);
}
