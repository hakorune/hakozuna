#include "hz12_live_footprint.h"

#if HZ12_LIVE_FOOTPRINT_DIAG

#include "hz12_size_class.h"
#include "hz12_span.h"
#include "hz12_sys_alloc.h"

#include <stdatomic.h>
#include <stdio.h>

static _Atomic uint64_t hz12_live_alloc_count[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_free_count[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_current[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_high_water[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_bytes_current[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_bytes_high_water[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_req_bytes_current[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_req_bytes_high_water[HZ12_CLASS_COUNT];
static _Atomic uint64_t hz12_live_req_bytes_alloc[HZ12_CLASS_COUNT];
static _Atomic(uint32_t*) hz12_live_req_by_span[HZ12_SPAN_COUNT];

static void hz12_live_max_update(_Atomic uint64_t* slot, uint64_t value) {
  uint64_t old = atomic_load_explicit(slot, memory_order_relaxed);
  while (old < value &&
         !atomic_compare_exchange_weak_explicit(slot, &old, value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}

static uint32_t* hz12_live_request_table(size_t span_id, uint8_t class_id) {
  uint32_t* table =
      atomic_load_explicit(&hz12_live_req_by_span[span_id],
                           memory_order_acquire);
  if (table) {
    return table;
  }
  size_t slot = hz12_class_slot_size(class_id);
  size_t slots = slot == 0u ? 0u : HZ12_SPAN_BYTES / slot;
  uint32_t* fresh = (uint32_t*)hz12_sys_calloc(slots, sizeof(uint32_t));
  if (!fresh) {
    return NULL;
  }
  uint32_t* expected = NULL;
  if (atomic_compare_exchange_strong_explicit(&hz12_live_req_by_span[span_id],
                                              &expected, fresh,
                                              memory_order_release,
                                              memory_order_acquire)) {
    return fresh;
  }
  hz12_sys_free(fresh);
  return expected;
}

static uint32_t* hz12_live_request_slot(void* ptr, uint8_t class_id) {
  if (!hz12_arena_base || !ptr || class_id >= HZ12_CLASS_COUNT) {
    return NULL;
  }
  uintptr_t p = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)hz12_arena_base;
  uintptr_t off = p - base;
  if (p < base || off >= (uintptr_t)HZ12_ARENA_BYTES) {
    return NULL;
  }
  size_t span_id = (size_t)(off >> HZ12_SPAN_SHIFT);
  size_t slot = hz12_class_slot_size(class_id);
  if (slot == 0u) {
    return NULL;
  }
  size_t slot_id = (size_t)((off & (HZ12_SPAN_BYTES - 1u)) / slot);
  uint32_t* table = hz12_live_request_table(span_id, class_id);
  return table ? table + slot_id : NULL;
}

void hz12_live_footprint_alloc(uint8_t class_id, void* ptr, size_t size) {
  if (class_id >= HZ12_CLASS_COUNT) {
    return;
  }
  uint64_t slot = (uint64_t)hz12_class_slot_size(class_id);
  uint32_t req = size > UINT32_MAX ? UINT32_MAX : (uint32_t)size;
  uint32_t* req_slot = hz12_live_request_slot(ptr, class_id);
  if (!req_slot) {
    req = 0u;
  }
  uint64_t live =
      atomic_fetch_add_explicit(&hz12_live_current[class_id], 1u,
                                memory_order_relaxed) +
      1u;
  uint64_t bytes =
      atomic_fetch_add_explicit(&hz12_live_bytes_current[class_id], slot,
                                memory_order_relaxed) +
      slot;
  atomic_fetch_add_explicit(&hz12_live_alloc_count[class_id], 1u,
                            memory_order_relaxed);
  atomic_fetch_add_explicit(&hz12_live_req_bytes_alloc[class_id], req,
                            memory_order_relaxed);
  hz12_live_max_update(&hz12_live_high_water[class_id], live);
  hz12_live_max_update(&hz12_live_bytes_high_water[class_id], bytes);
  uint64_t req_bytes =
      atomic_fetch_add_explicit(&hz12_live_req_bytes_current[class_id], req,
                                memory_order_relaxed) +
      req;
  hz12_live_max_update(&hz12_live_req_bytes_high_water[class_id], req_bytes);
  if (req_slot) {
    *req_slot = req;
  }
}

void hz12_live_footprint_free(uint8_t class_id, void* ptr) {
  if (class_id >= HZ12_CLASS_COUNT) {
    return;
  }
  uint64_t slot = (uint64_t)hz12_class_slot_size(class_id);
  uint32_t req = 0u;
  uint32_t* req_slot = hz12_live_request_slot(ptr, class_id);
  if (req_slot) {
    req = *req_slot;
    *req_slot = 0u;
  }
  atomic_fetch_add_explicit(&hz12_live_free_count[class_id], 1u,
                            memory_order_relaxed);
  atomic_fetch_sub_explicit(&hz12_live_current[class_id], 1u,
                            memory_order_relaxed);
  atomic_fetch_sub_explicit(&hz12_live_bytes_current[class_id], slot,
                            memory_order_relaxed);
  atomic_fetch_sub_explicit(&hz12_live_req_bytes_current[class_id], req,
                            memory_order_relaxed);
}

void hz12_live_footprint_dump(void) {
  fprintf(stderr, "hz12_live_footprint_begin span_bytes=%u\n",
          (unsigned)HZ12_SPAN_BYTES);
  for (uint32_t class_id = 0u; class_id < HZ12_CLASS_COUNT; ++class_id) {
    uint64_t alloc =
        atomic_load_explicit(&hz12_live_alloc_count[class_id],
                             memory_order_relaxed);
    uint64_t free =
        atomic_load_explicit(&hz12_live_free_count[class_id],
                             memory_order_relaxed);
    uint64_t live =
        atomic_load_explicit(&hz12_live_current[class_id],
                             memory_order_relaxed);
    uint64_t live_high =
        atomic_load_explicit(&hz12_live_high_water[class_id],
                             memory_order_relaxed);
    uint64_t bytes_live =
        atomic_load_explicit(&hz12_live_bytes_current[class_id],
                             memory_order_relaxed);
    uint64_t bytes_high =
        atomic_load_explicit(&hz12_live_bytes_high_water[class_id],
                             memory_order_relaxed);
    uint64_t req_bytes_live =
        atomic_load_explicit(&hz12_live_req_bytes_current[class_id],
                             memory_order_relaxed);
    uint64_t req_bytes_high =
        atomic_load_explicit(&hz12_live_req_bytes_high_water[class_id],
                             memory_order_relaxed);
    uint64_t req_bytes_alloc =
        atomic_load_explicit(&hz12_live_req_bytes_alloc[class_id],
                             memory_order_relaxed);
    if (alloc == 0u && free == 0u && live == 0u && live_high == 0u &&
        bytes_live == 0u && bytes_high == 0u && req_bytes_live == 0u &&
        req_bytes_high == 0u && req_bytes_alloc == 0u) {
      continue;
    }
    fprintf(stderr,
            "hz12_live_footprint_class class=%u slot=%zu alloc=%llu "
            "free=%llu live=%llu live_high=%llu bytes_live=%llu "
            "bytes_high=%llu req_bytes_live=%llu req_bytes_high=%llu "
            "req_bytes_alloc=%llu\n",
            (unsigned)class_id, hz12_class_slot_size((uint8_t)class_id),
            (unsigned long long)alloc, (unsigned long long)free,
            (unsigned long long)live, (unsigned long long)live_high,
            (unsigned long long)bytes_live, (unsigned long long)bytes_high,
            (unsigned long long)req_bytes_live,
            (unsigned long long)req_bytes_high,
            (unsigned long long)req_bytes_alloc);
  }
  fprintf(stderr, "hz12_live_footprint_end\n");
}

#endif /* HZ12_LIVE_FOOTPRINT_DIAG */
