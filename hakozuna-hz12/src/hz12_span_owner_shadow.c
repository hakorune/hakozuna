#include "hz12_span_owner_shadow.h"

#include "hz12_span.h"

#include <stdatomic.h>

static atomic_uint_fast64_t h12_span_owner_tokens[HZ12_SPAN_COUNT];

static uint64_t h12_span_owner_pack(H12OwnerToken owner) {
  return ((uint64_t)owner.generation << 32) | owner.slot;
}

static int h12_span_owner_id(const void* ptr, uint32_t* out) {
  uintptr_t value;
  uintptr_t base;
  uintptr_t offset;
  if (!ptr || !out || !hz12_arena_base) return 0;
  value = (uintptr_t)ptr;
  base = (uintptr_t)hz12_arena_base;
  if (value < base) return 0;
  offset = value - base;
  if (offset >= (uintptr_t)HZ12_ARENA_BYTES) return 0;
  *out = (uint32_t)(offset >> HZ12_SPAN_SHIFT);
  return 1;
}

void h12_span_owner_shadow_reset(void) {
  uint32_t i;
  for (i = 0u; i < HZ12_SPAN_COUNT; ++i) {
    atomic_store_explicit(&h12_span_owner_tokens[i], 0u,
                          memory_order_relaxed);
  }
}

int h12_span_owner_shadow_assign(const void* ptr, H12OwnerToken owner) {
  uint32_t span_id;
  if (owner.generation == 0u || !h12_span_owner_id(ptr, &span_id)) return 0;
  atomic_store_explicit(&h12_span_owner_tokens[span_id],
                        h12_span_owner_pack(owner), memory_order_release);
  return 1;
}

int h12_span_owner_shadow_query(uint32_t span_id, H12OwnerToken* out) {
  uint64_t packed;
  if (!out || span_id >= HZ12_SPAN_COUNT) return 0;
  packed = atomic_load_explicit(&h12_span_owner_tokens[span_id],
                                memory_order_acquire);
  if (packed == 0u) return 0;
  out->slot = (uint32_t)packed;
  out->generation = (uint32_t)(packed >> 32);
  return out->generation != 0u;
}
