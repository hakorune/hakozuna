#include "hz6_transfer.h"
#include "../include/hz6_stats_snapshot.h"

#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
#include <stdatomic.h>
#endif

#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
static int hz6_transfer_class_valid(uint16_t class_id) {
  return class_id < HZ6_FRONT_CACHE_CLASS_COUNT;
}

static uint32_t hz6_transfer_load_u32(const _Atomic uint32_t* value) {
  return atomic_load_explicit(value, memory_order_acquire);
}

#if HZ6_TRANSFER_CLASS_PRESENCE_OBSERVE_L1
static void hz6_transfer_inc_u32(_Atomic uint32_t* value) {
  atomic_fetch_add_explicit(value, 1u, memory_order_release);
}

#define HZ6_TRANSFER_PRESENCE_NOTE(cache, field) \
  hz6_transfer_inc_u32(&(cache)->field)
#else
#define HZ6_TRANSFER_PRESENCE_NOTE(cache, field) ((void)(cache))
#endif

static int hz6_transfer_dec_u32(_Atomic uint32_t* value) {
  uint32_t current = hz6_transfer_load_u32(value);
  while (current != 0) {
    if (atomic_compare_exchange_weak_explicit(
            value, &current, current - 1u, memory_order_acq_rel,
            memory_order_acquire)) {
      return 1;
    }
  }
  return 0;
}

static int hz6_transfer_object_counts_for_presence(
    const Hz6TransferObject* object) {
  return object && object->ptr &&
         hz6_transfer_class_valid(object->class_id);
}

static void hz6_transfer_presence_increment(Hz6TransferCache* cache,
                                            const Hz6TransferObject* object) {
  if (!cache || !object) {
    return;
  }
  if (!object->ptr || object->class_id == UINT16_MAX) {
    return;
  }
  if (!hz6_transfer_class_valid(object->class_id)) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_invalid_class);
    return;
  }
  uint32_t previous = atomic_fetch_add_explicit(
      &cache->class_count[object->class_id], 1u, memory_order_release);
  if ((size_t)previous + 1u > cache->capacity) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_over_capacity);
  }
}

static void hz6_transfer_presence_decrement(Hz6TransferCache* cache,
                                            const Hz6TransferObject* object) {
  if (!cache || !object) {
    return;
  }
  if (!object->ptr || object->class_id == UINT16_MAX) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_placeholder_counted);
    return;
  }
  if (!hz6_transfer_class_valid(object->class_id)) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_invalid_class);
    return;
  }
  if (!hz6_transfer_dec_u32(&cache->class_count[object->class_id])) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_underflow);
  }
}

#if HZ6_DIAGNOSTIC_PROBES
static int hz6_transfer_dense_has_class(const Hz6TransferCache* cache,
                                        uint16_t class_id) {
  if (!cache || !cache->objects) {
    return 0;
  }
  for (size_t i = 0; i < cache->count; ++i) {
    if (cache->objects[i].ptr && cache->objects[i].class_id == class_id) {
      return 1;
    }
  }
  return 0;
}
#endif
#endif

void hz6_transfer_init(Hz6TransferCache* cache,
                       Hz6TransferObject* objects,
                       size_t capacity) {
  if (!cache) {
    return;
  }
  cache->objects = objects;
  cache->capacity = capacity;
  cache->count = 0;
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    atomic_store_explicit(&cache->class_count[i], 0, memory_order_relaxed);
  }
#if HZ6_TRANSFER_CLASS_PRESENCE_OBSERVE_L1
  atomic_store_explicit(&cache->presence_gate_check, 0, memory_order_relaxed);
  atomic_store_explicit(&cache->presence_gate_hit, 0, memory_order_relaxed);
  atomic_store_explicit(&cache->presence_gate_miss, 0, memory_order_relaxed);
  atomic_store_explicit(&cache->presence_invalid_class, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_underflow, 0, memory_order_relaxed);
  atomic_store_explicit(&cache->presence_over_capacity, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_placeholder_counted, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_double_increment, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_double_decrement, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_sum_mismatch, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_scan_mismatch, 0,
                        memory_order_relaxed);
  atomic_store_explicit(&cache->presence_false_zero_shadow, 0,
                        memory_order_relaxed);
#endif
#endif
}

int hz6_transfer_push(Hz6TransferCache* cache, Hz6TransferObject object) {
  if (!cache || !cache->objects || !object.ptr ||
      cache->count >= cache->capacity) {
    return 0;
  }
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
  hz6_transfer_presence_increment(cache, &object);
#endif
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
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
    if (hz6_transfer_object_counts_for_presence(
            &cache->objects[reservation->index])) {
      HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_double_increment);
    }
#endif
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
    hz6_transfer_presence_increment(cache, &object);
#endif
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

#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
  if (!hz6_transfer_class_valid(class_id)) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_invalid_class);
    return 0;
  }
  if (cache->count == 0) {
    return 0;
  }
  if (!hz6_transfer_class_maybe_present(cache, class_id)) {
#if HZ6_DIAGNOSTIC_PROBES
    if (hz6_transfer_dense_has_class(cache, class_id)) {
      HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_false_zero_shadow);
    }
#endif
    return 0;
  }
#endif

  for (size_t i = cache->count; i > 0; --i) {
    size_t index = i - 1;
    if (cache->objects[index].class_id != class_id) {
      continue;
    }
    *out = cache->objects[index];
    cache->objects[index] = cache->objects[--cache->count];
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
    hz6_transfer_presence_decrement(cache, out);
#endif
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

int hz6_transfer_class_maybe_present(Hz6TransferCache* cache,
                                     uint16_t class_id) {
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1
  if (!cache || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    if (cache) {
      HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_invalid_class);
    }
    return 0;
  }
  if (cache->count == 0) {
    return 0;
  }
  HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_gate_check);
  if (hz6_transfer_load_u32(&cache->class_count[class_id]) == 0) {
    HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_gate_miss);
    return 0;
  }
  HZ6_TRANSFER_PRESENCE_NOTE(cache, presence_gate_hit);
  return 1;
#else
  (void)cache;
  (void)class_id;
  return 1;
#endif
}

void hz6_transfer_note_class_presence_stats(
    const Hz6TransferCache* cache,
    Hz6StatsSnapshot* snapshot) {
#if HZ6_TRANSFER_CLASS_PRESENCE_GATE_L1 && \
    HZ6_TRANSFER_CLASS_PRESENCE_OBSERVE_L1
  if (!cache || !snapshot) {
    return;
  }
  snapshot->transfer_class_presence_gate_check +=
      hz6_transfer_load_u32(&cache->presence_gate_check);
  snapshot->transfer_class_presence_gate_hit +=
      hz6_transfer_load_u32(&cache->presence_gate_hit);
  snapshot->transfer_class_presence_gate_miss +=
      hz6_transfer_load_u32(&cache->presence_gate_miss);
  snapshot->transfer_class_presence_invalid_class +=
      hz6_transfer_load_u32(&cache->presence_invalid_class);
  snapshot->transfer_class_presence_underflow +=
      hz6_transfer_load_u32(&cache->presence_underflow);
  snapshot->transfer_class_presence_over_capacity +=
      hz6_transfer_load_u32(&cache->presence_over_capacity);
  snapshot->transfer_class_presence_placeholder_counted +=
      hz6_transfer_load_u32(&cache->presence_placeholder_counted);
  snapshot->transfer_class_presence_double_increment +=
      hz6_transfer_load_u32(&cache->presence_double_increment);
  snapshot->transfer_class_presence_double_decrement +=
      hz6_transfer_load_u32(&cache->presence_double_decrement);
  snapshot->transfer_class_presence_sum_mismatch +=
      hz6_transfer_load_u32(&cache->presence_sum_mismatch);
  snapshot->transfer_class_presence_scan_mismatch +=
      hz6_transfer_load_u32(&cache->presence_scan_mismatch);
  snapshot->transfer_class_presence_false_zero_shadow +=
      hz6_transfer_load_u32(&cache->presence_false_zero_shadow);

  size_t sum = 0;
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    size_t hint_count = hz6_transfer_load_u32(&cache->class_count[i]);
    size_t scan_count = hz6_transfer_count_class(cache, (uint16_t)i);
    sum += hint_count;
    if (hint_count != scan_count) {
      ++snapshot->transfer_class_presence_scan_mismatch;
    }
  }
  if (sum != cache->count) {
    ++snapshot->transfer_class_presence_sum_mismatch;
  }
#else
  (void)cache;
  (void)snapshot;
#endif
}
