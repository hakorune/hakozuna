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

int hz6_transfer_reserve(Hz6TransferCache* cache,
                         Hz6TransferReservation* out) {
  if (!cache || !cache->objects || !out ||
      cache->count >= cache->capacity) {
    return 0;
  }

  Hz6TransferObject placeholder = {0};
  placeholder.class_id = UINT16_MAX;
  out->cache = cache;
  out->index = cache->count;
  out->reserved = 1;
  cache->objects[cache->count++] = placeholder;
  return 1;
}

void hz6_transfer_cancel(Hz6TransferReservation* reservation) {
  if (!reservation || !reservation->reserved || !reservation->cache) {
    return;
  }
  Hz6TransferCache* cache = reservation->cache;
  if (reservation->index < cache->count) {
    cache->objects[reservation->index] = cache->objects[--cache->count];
  }
  reservation->cache = NULL;
  reservation->index = 0;
  reservation->reserved = 0;
}

void hz6_transfer_commit(Hz6TransferReservation* reservation,
                         Hz6TransferObject object) {
  if (!reservation || !reservation->reserved || !reservation->cache) {
    return;
  }
  Hz6TransferCache* cache = reservation->cache;
  if (reservation->index < cache->count) {
    cache->objects[reservation->index] = object;
  }
  reservation->cache = NULL;
  reservation->index = 0;
  reservation->reserved = 0;
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

size_t hz6_transfer_count_class(const Hz6TransferCache* cache,
                                uint16_t class_id) {
  if (!cache || !cache->objects) {
    return 0;
  }

  size_t count = 0;
  for (size_t i = 0; i < cache->count; ++i) {
    if (cache->objects[i].class_id == class_id) {
      ++count;
    }
  }
  return count;
}
