#include "h8_internal.h"

#include <stdlib.h>

#if defined(H8_ENABLE_DEBUG_STATS)
static uint32_t h8_slot_shadow_load(H8Span* span, size_t slot) {
  if (!span->slot_state_shadow) {
    return H8_SLOT_POISON;
  }
  return atomic_load_explicit(&span->slot_state_shadow[slot], memory_order_acquire);
}

void h8_slot_shadow_set_allocated(H8Span* span, size_t slot) {
  if (!span->slot_state_shadow) {
    return;
  }
  atomic_store_explicit(&span->slot_state_shadow[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
}

void h8_slot_shadow_set_free(H8Span* span, size_t slot, uint32_t next) {
  if (!span->slot_state_shadow) {
    return;
  }
  atomic_store_explicit(&span->slot_state_shadow[slot], h8_slot_state_free(next),
                        memory_order_release);
}

void h8_slot_shadow_expect(H8Span* span, size_t slot, uint32_t tag) {
  uint32_t state = h8_slot_shadow_load(span, slot);
  if (h8_slot_state_tag(state) != tag) {
    H8_DEBUG_INC(slot_shadow_invalid_mismatch);
  }
}

static void h8_slot_shadow_mark_seen(uint8_t* seen, size_t slot, size_t count) {
  if (slot >= count) {
    H8_DEBUG_INC(slot_shadow_bad_next);
    return;
  }
  if (seen[slot]) {
    H8_DEBUG_INC(slot_shadow_free_duplicate);
  }
  seen[slot] = 1;
}

static void h8_slot_shadow_verify_chain(H8Span* span, uint8_t* seen) {
  size_t guard = 0;
  uint32_t slot = atomic_load_explicit(&span->local_free_head, memory_order_acquire);
  while (slot != UINT32_MAX) {
    if (slot >= span->slot_count) {
      H8_DEBUG_INC(slot_shadow_bad_next);
      return;
    }
    if (++guard > span->slot_count) {
      H8_DEBUG_INC(slot_shadow_free_cycle);
      return;
    }
    h8_slot_shadow_mark_seen(seen, slot, span->slot_count);
    uint32_t state = h8_slot_shadow_load(span, slot);
    if (h8_slot_state_tag(state) != (H8_SLOT_FREE >> H8_SLOT_TAG_SHIFT)) {
      H8_DEBUG_INC(slot_shadow_invalid_mismatch);
      return;
    }
    slot = h8_slot_state_decode_next(h8_slot_state_payload(state));
  }
}

void h8_slot_shadow_verify_span(H8Span* span) {
  if (!span || !span->slot_state_shadow) {
    return;
  }
  uint8_t* seen = h8_sys_calloc(span->slot_count, sizeof(uint8_t));
  if (!seen) {
    abort();
  }
  h8_slot_shadow_verify_chain(span, seen);

  size_t allocated = 0;
  size_t live_count = 0;
  size_t pending_count = 0;
  uint32_t bump = atomic_load_explicit(&span->bump_index, memory_order_acquire);
  if (bump > span->slot_count) {
    H8_DEBUG_INC(slot_shadow_nonvirgin_above_bump);
  }

  for (size_t slot = 0; slot < span->slot_count; ++slot) {
    bool live = h8_bitmap_test(span->live_bits, slot);
    bool pending = h8_bitmap_test(span->pending_bits, slot);
    uint32_t state = h8_slot_shadow_load(span, slot);
    uint32_t tag = h8_slot_state_tag(state);
    if (live) {
      ++live_count;
    }
    if (pending) {
      ++pending_count;
    }
    if (tag == (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
      ++allocated;
      if (!live) {
        H8_DEBUG_INC(slot_shadow_valid_mismatch);
      }
    } else if (live) {
      H8_DEBUG_INC(slot_shadow_invalid_mismatch);
    }
    if (pending && tag != (H8_SLOT_ALLOCATED >> H8_SLOT_TAG_SHIFT)) {
      H8_DEBUG_INC(slot_shadow_pending_nonallocated);
    }
    if (tag == (H8_SLOT_FREE >> H8_SLOT_TAG_SHIFT)) {
      if (!seen[slot]) {
        H8_DEBUG_INC(slot_shadow_free_unreachable);
      }
      if (live || pending) {
        H8_DEBUG_INC(slot_shadow_invalid_mismatch);
      }
      uint32_t next = h8_slot_state_decode_next(h8_slot_state_payload(state));
      if (next != UINT32_MAX && next >= span->slot_count) {
        H8_DEBUG_INC(slot_shadow_bad_next);
      }
    } else if (tag == (H8_SLOT_NEVER_USED >> H8_SLOT_TAG_SHIFT)) {
      if (slot < bump) {
        H8_DEBUG_INC(slot_shadow_never_used_below_bump);
      }
      if (live || pending) {
        H8_DEBUG_INC(slot_shadow_invalid_mismatch);
      }
    } else if (tag == (H8_SLOT_POISON >> H8_SLOT_TAG_SHIFT)) {
      H8_DEBUG_INC(slot_shadow_reserved_quiescent);
    }
    if (slot >= bump && tag != (H8_SLOT_NEVER_USED >> H8_SLOT_TAG_SHIFT)) {
      H8_DEBUG_INC(slot_shadow_nonvirgin_above_bump);
    }
  }
  size_t used = atomic_load_explicit(&span->used_count, memory_order_acquire);
  size_t pending_seen = atomic_load_explicit(&span->pending_count, memory_order_acquire);
  if (used != allocated || used != live_count || pending_seen != pending_count ||
      pending_seen > used) {
    H8_DEBUG_INC(slot_shadow_used_mismatch);
  }
  h8_sys_free(seen);
}
#else
void h8_slot_shadow_set_allocated(H8Span* span, size_t slot) {
  (void)span;
  (void)slot;
}

void h8_slot_shadow_set_free(H8Span* span, size_t slot, uint32_t next) {
  (void)span;
  (void)slot;
  (void)next;
}

void h8_slot_shadow_expect(H8Span* span, size_t slot, uint32_t tag) {
  (void)span;
  (void)slot;
  (void)tag;
}

void h8_slot_shadow_verify_span(H8Span* span) {
  (void)span;
}
#endif
