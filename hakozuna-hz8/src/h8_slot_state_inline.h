#ifndef H8_SLOT_STATE_INLINE_H
#define H8_SLOT_STATE_INLINE_H

/*
 * Internal leaf included by h8_internal.h after H8Span is fully defined.
 * Keep these helpers inline: they are on malloc/free/remote publish paths.
 */

#define H8_SLOT_TAG_SHIFT 30u
#define H8_SLOT_PAYLOAD_MASK ((UINT32_C(1) << H8_SLOT_TAG_SHIFT) - 1u)
#define H8_SLOT_NONE H8_SLOT_PAYLOAD_MASK
#define H8_SLOT_NEVER_USED (UINT32_C(0) << H8_SLOT_TAG_SHIFT)
#define H8_SLOT_ALLOCATED (UINT32_C(1) << H8_SLOT_TAG_SHIFT)
#define H8_SLOT_FREE (UINT32_C(2) << H8_SLOT_TAG_SHIFT)
#define H8_SLOT_POISON (UINT32_C(3) << H8_SLOT_TAG_SHIFT)

static inline uint32_t h8_slot_state_tag(uint32_t state) {
  return state >> H8_SLOT_TAG_SHIFT;
}

static inline uint32_t h8_slot_state_payload(uint32_t state) {
  return state & H8_SLOT_PAYLOAD_MASK;
}

static inline uint32_t h8_slot_state_next_payload(uint32_t next) {
  return next;
}

static inline uint32_t h8_slot_state_decode_next(uint32_t payload) {
  return payload;
}

static inline uint32_t h8_slot_state_free(uint32_t next) {
  return H8_SLOT_FREE | h8_slot_state_next_payload(next);
}

static inline uint32_t h8_slot_state_load_hot(H8Span* span, size_t slot) {
  return atomic_load_explicit(&span->slot_state[slot], memory_order_acquire);
}

static inline uint32_t h8_slot_state_load_ptr_hot(_Atomic uint32_t* slot_state,
                                                  size_t slot) {
  return atomic_load_explicit(&slot_state[slot], memory_order_acquire);
}

static inline void h8_slot_state_store_allocated_hot(H8Span* span, size_t slot) {
  atomic_store_explicit(&span->slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
}

static inline void h8_slot_state_store_allocated_ptr_hot(
    _Atomic uint32_t* slot_state, size_t slot) {
  atomic_store_explicit(&slot_state[slot], H8_SLOT_ALLOCATED,
                        memory_order_release);
}

static inline void h8_slot_state_store_free_hot(H8Span* span, size_t slot,
                                                uint32_t next) {
  atomic_store_explicit(&span->slot_state[slot], h8_slot_state_free(next),
                        memory_order_release);
}

#endif
