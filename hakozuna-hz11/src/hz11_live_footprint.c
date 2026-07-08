#include "hz11_live_footprint.h"

#if HZ11_LIVE_FOOTPRINT_DIAG

#include "hz11_size_class.h"
#include "hz11_span.h"

#include <stdatomic.h>
#include <stdio.h>

static _Atomic uint64_t hz11_live_alloc_count[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_live_free_count[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_live_current[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_live_high_water[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_live_bytes_current[HZ11_CLASS_COUNT];
static _Atomic uint64_t hz11_live_bytes_high_water[HZ11_CLASS_COUNT];

static void hz11_live_max_update(_Atomic uint64_t* slot, uint64_t value) {
  uint64_t old = atomic_load_explicit(slot, memory_order_relaxed);
  while (old < value &&
         !atomic_compare_exchange_weak_explicit(slot, &old, value,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
  }
}

void hz11_live_footprint_alloc(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return;
  }
  uint64_t slot = (uint64_t)hz11_class_slot_size(class_id);
  uint64_t live =
      atomic_fetch_add_explicit(&hz11_live_current[class_id], 1u,
                                memory_order_relaxed) +
      1u;
  uint64_t bytes =
      atomic_fetch_add_explicit(&hz11_live_bytes_current[class_id], slot,
                                memory_order_relaxed) +
      slot;
  atomic_fetch_add_explicit(&hz11_live_alloc_count[class_id], 1u,
                            memory_order_relaxed);
  hz11_live_max_update(&hz11_live_high_water[class_id], live);
  hz11_live_max_update(&hz11_live_bytes_high_water[class_id], bytes);
}

void hz11_live_footprint_free(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return;
  }
  uint64_t slot = (uint64_t)hz11_class_slot_size(class_id);
  atomic_fetch_add_explicit(&hz11_live_free_count[class_id], 1u,
                            memory_order_relaxed);
  atomic_fetch_sub_explicit(&hz11_live_current[class_id], 1u,
                            memory_order_relaxed);
  atomic_fetch_sub_explicit(&hz11_live_bytes_current[class_id], slot,
                            memory_order_relaxed);
}

void hz11_live_footprint_dump(void) {
  fprintf(stderr, "hz11_live_footprint_begin span_bytes=%u\n",
          (unsigned)HZ11_SPAN_BYTES);
  for (uint32_t class_id = 0u; class_id < HZ11_CLASS_COUNT; ++class_id) {
    uint64_t alloc =
        atomic_load_explicit(&hz11_live_alloc_count[class_id],
                             memory_order_relaxed);
    uint64_t free =
        atomic_load_explicit(&hz11_live_free_count[class_id],
                             memory_order_relaxed);
    uint64_t live =
        atomic_load_explicit(&hz11_live_current[class_id],
                             memory_order_relaxed);
    uint64_t live_high =
        atomic_load_explicit(&hz11_live_high_water[class_id],
                             memory_order_relaxed);
    uint64_t bytes_live =
        atomic_load_explicit(&hz11_live_bytes_current[class_id],
                             memory_order_relaxed);
    uint64_t bytes_high =
        atomic_load_explicit(&hz11_live_bytes_high_water[class_id],
                             memory_order_relaxed);
    if (alloc == 0u && free == 0u && live == 0u && live_high == 0u &&
        bytes_live == 0u && bytes_high == 0u) {
      continue;
    }
    fprintf(stderr,
            "hz11_live_footprint_class class=%u slot=%zu alloc=%llu "
            "free=%llu live=%llu live_high=%llu bytes_live=%llu "
            "bytes_high=%llu\n",
            (unsigned)class_id, hz11_class_slot_size((uint8_t)class_id),
            (unsigned long long)alloc, (unsigned long long)free,
            (unsigned long long)live, (unsigned long long)live_high,
            (unsigned long long)bytes_live, (unsigned long long)bytes_high);
  }
  fprintf(stderr, "hz11_live_footprint_end\n");
}

#endif /* HZ11_LIVE_FOOTPRINT_DIAG */
