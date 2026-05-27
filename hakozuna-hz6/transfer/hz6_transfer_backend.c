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

int hz6_transfer_backend_push(Hz6TransferBackend* backend,
                              Hz6TransferObject object) {
  if (!backend) {
    return 0;
  }
  if (backend->kind == HZ6_TRANSFER_BACKEND_SINGLE_CACHE) {
    return hz6_transfer_push(&backend->single_cache, object);
  }
  if (backend->kind != HZ6_TRANSFER_BACKEND_SHARDED_CACHE ||
      backend->shard_count == 0) {
    return 0;
  }

  size_t start = backend->next_push_shard % backend->shard_count;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    size_t shard_index = (start + i) % backend->shard_count;
    if (hz6_transfer_push(&backend->shard[shard_index], object)) {
      backend->next_push_shard = (shard_index + 1u) % backend->shard_count;
      return 1;
    }
  }
  return 0;
}

int hz6_transfer_backend_pop(Hz6TransferBackend* backend,
                             uint16_t class_id,
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

  size_t start = ((size_t)class_id) % backend->shard_count;
  for (size_t i = 0; i < backend->shard_count; ++i) {
    size_t shard_index = (start + i) % backend->shard_count;
    if (hz6_transfer_pop(&backend->shard[shard_index], class_id, out)) {
      return 1;
    }
  }
  return 0;
}

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
