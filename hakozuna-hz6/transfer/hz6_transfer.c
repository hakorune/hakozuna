#include "hz6_transfer.h"

void hz6_transfer_init(Hz6TransferCache* cache,
                       Hz6TransferObject* objects,
                       size_t capacity) {
  if (!cache) {
    return;
  }
  cache->objects = objects;
  cache->capacity = capacity;
  cache->count = 0;
}

int hz6_transfer_push(Hz6TransferCache* cache, Hz6TransferObject object) {
  if (!cache || !cache->objects || !object.ptr ||
      cache->count >= cache->capacity) {
    return 0;
  }
  cache->objects[cache->count++] = object;
  return 1;
}

int hz6_transfer_pop(Hz6TransferCache* cache,
                     uint16_t class_id,
                     Hz6TransferObject* out) {
  if (!cache || !cache->objects || !out) {
    return 0;
  }

  for (size_t i = cache->count; i > 0; --i) {
    size_t index = i - 1;
    if (cache->objects[index].class_id != class_id) {
      continue;
    }
    *out = cache->objects[index];
    cache->objects[index] = cache->objects[--cache->count];
    return 1;
  }

  return 0;
}

size_t hz6_transfer_count(const Hz6TransferCache* cache) {
  return cache ? cache->count : 0;
}

