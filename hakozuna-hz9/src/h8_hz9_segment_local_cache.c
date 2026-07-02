#include "h8_internal.h"

#if defined(H9_SEGMENT_LOCAL_CACHE_L0)

#include <string.h>

typedef enum H9SegmentState {
  H9_SEGMENT_LOCAL = 0,
  H9_SEGMENT_REMOTE_SEEN = 1,
  H9_SEGMENT_RETIRED = 2
} H9SegmentState;

typedef struct H9SegmentLocalClass {
  uint64_t local_free_bits;
  uint64_t local_alloc_bits;
  uint64_t remote_pending_bits;
  H9SegmentState state;
} H9SegmentLocalClass;

typedef struct H9SegmentLocalState {
  H9SegmentLocalClass classes[H8_MEDIUM_CLASS_COUNT];
  uint64_t touched_class_bits;
} H9SegmentLocalState;

static _Thread_local H9SegmentLocalState h9_segment_tls;

static bool h9_segment_slot_valid(uint32_t class_id, uint32_t slot) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  return spec && slot < spec->slot_count && slot < 64u;
}

static H9SegmentLocalClass* h9_segment_class(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return &h9_segment_tls.classes[class_id];
}

void h9_segment_local_cache_debug_reset(void) {
  memset(&h9_segment_tls, 0, sizeof(h9_segment_tls));
}

size_t h9_segment_local_cache_debug_state_size(void) {
  return sizeof(h9_segment_tls);
}

bool h9_segment_local_cache_debug_put(uint32_t class_id, uint32_t slot) {
  if (!h9_segment_slot_valid(class_id, slot)) {
    return false;
  }
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_LOCAL) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((cls->local_free_bits & bit) != 0u ||
      (cls->local_alloc_bits & bit) != 0u ||
      (cls->remote_pending_bits & bit) != 0u) {
    return false;
  }
  cls->local_free_bits |= bit;
  h9_segment_tls.touched_class_bits |= UINT64_C(1) << class_id;
  return true;
}

bool h9_segment_local_cache_debug_take(uint32_t class_id,
                                       uint32_t* slot_out) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_LOCAL ||
      cls->local_free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(cls->local_free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  cls->local_free_bits &= ~bit;
  cls->local_alloc_bits |= bit;
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

bool h9_segment_local_cache_debug_free_allocated(uint32_t class_id,
                                                 uint32_t slot) {
  if (!h9_segment_slot_valid(class_id, slot)) {
    return false;
  }
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_LOCAL) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((cls->local_alloc_bits & bit) == 0u ||
      (cls->local_free_bits & bit) != 0u ||
      (cls->remote_pending_bits & bit) != 0u) {
    return false;
  }
  cls->local_alloc_bits &= ~bit;
  cls->local_free_bits |= bit;
  return true;
}

bool h9_segment_local_cache_debug_remote_mark(uint32_t class_id,
                                              uint32_t slot) {
  if (!h9_segment_slot_valid(class_id, slot)) {
    return false;
  }
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state == H9_SEGMENT_RETIRED) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((cls->remote_pending_bits & bit) != 0u) {
    return false;
  }
  cls->remote_pending_bits |= bit;
  cls->local_free_bits &= ~bit;
  cls->local_alloc_bits &= ~bit;
  cls->state = H9_SEGMENT_REMOTE_SEEN;
  return true;
}

bool h9_segment_local_cache_debug_drain_remote(uint32_t class_id,
                                               uint64_t* pending_out) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_REMOTE_SEEN ||
      cls->remote_pending_bits == 0u) {
    return false;
  }
  if (pending_out) {
    *pending_out = cls->remote_pending_bits;
  }
  cls->local_free_bits = 0u;
  cls->local_alloc_bits = 0u;
  cls->remote_pending_bits = 0u;
  cls->state = H9_SEGMENT_RETIRED;
  return true;
}

bool h9_segment_local_cache_debug_retire(uint32_t class_id) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls) {
    return false;
  }
  cls->local_free_bits = 0u;
  cls->local_alloc_bits = 0u;
  cls->remote_pending_bits = 0u;
  cls->state = H9_SEGMENT_RETIRED;
  return true;
}

uint64_t h9_segment_local_cache_debug_release_all(void) {
  uint64_t released = h9_segment_tls.touched_class_bits;
  memset(&h9_segment_tls, 0, sizeof(h9_segment_tls));
  return released;
}

bool h9_segment_local_cache_debug_class_geometry(uint32_t class_id,
                                                 uint32_t* slot_size_out,
                                                 uint32_t* run_size_out,
                                                 uint16_t* slot_count_out) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_count == 0u || spec->slot_count > 64u) {
    return false;
  }
  if (slot_size_out) {
    *slot_size_out = spec->slot_size;
  }
  if (run_size_out) {
    *run_size_out = spec->run_size;
  }
  if (slot_count_out) {
    *slot_count_out = spec->slot_count;
  }
  return true;
}

bool h9_segment_local_cache_debug_class_capacity(uint32_t class_id,
                                                 size_t* payload_bytes_out,
                                                 size_t* slack_bytes_out) {
  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  if (!h9_segment_local_cache_debug_class_geometry(
          class_id, &slot_size, &run_size, &slot_count)) {
    return false;
  }
  size_t payload = (size_t)slot_size * (size_t)slot_count;
  if (payload > (size_t)run_size) {
    return false;
  }
  if (payload_bytes_out) {
    *payload_bytes_out = payload;
  }
  if (slack_bytes_out) {
    *slack_bytes_out = (size_t)run_size - payload;
  }
  return true;
}

uint32_t h9_segment_local_cache_debug_state(uint32_t class_id) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  return cls ? (uint32_t)cls->state : UINT32_MAX;
}

uint64_t h9_segment_local_cache_debug_free_bits(uint32_t class_id) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  return cls ? cls->local_free_bits : 0u;
}

uint64_t h9_segment_local_cache_debug_alloc_bits(uint32_t class_id) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  return cls ? cls->local_alloc_bits : 0u;
}

uint64_t h9_segment_local_cache_debug_remote_bits(uint32_t class_id) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  return cls ? cls->remote_pending_bits : 0u;
}

uint64_t h9_segment_local_cache_debug_touched_classes(void) {
  return h9_segment_tls.touched_class_bits;
}

#else
typedef int h9_segment_local_cache_translation_unit_silence;
#endif
