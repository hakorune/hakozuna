#include "h8_internal.h"

#if defined(H9_STATIC_LOCAL_PAGE_SCAFFOLD_L0)

#include <string.h>

typedef struct H9StaticLocalClass {
  uint64_t local_free_bits;
  uint64_t local_alloc_bits;
} H9StaticLocalClass;

typedef struct H9StaticLocalPageState {
  H9StaticLocalClass classes[H8_MEDIUM_CLASS_COUNT];
  uint64_t touched_class_bits;
} H9StaticLocalPageState;

static _Thread_local H9StaticLocalPageState h9_static_local_page_tls;

static bool h9_static_local_page_class_slot_valid(uint32_t class_id,
                                                  uint32_t slot) {
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  return spec && slot < spec->slot_count && slot < 64u;
}

void h9_static_local_page_debug_reset(void) {
  memset(&h9_static_local_page_tls, 0, sizeof(h9_static_local_page_tls));
}

size_t h9_static_local_page_debug_state_size(void) {
  return sizeof(h9_static_local_page_tls);
}

bool h9_static_local_page_debug_put(uint32_t class_id, uint32_t slot) {
  if (!h9_static_local_page_class_slot_valid(class_id, slot)) {
    return false;
  }
  H9StaticLocalClass* cls = &h9_static_local_page_tls.classes[class_id];
  uint64_t bit = UINT64_C(1) << slot;
  if ((cls->local_free_bits & bit) != 0u ||
      (cls->local_alloc_bits & bit) != 0u) {
    return false;
  }
  cls->local_free_bits |= bit;
  h9_static_local_page_tls.touched_class_bits |= UINT64_C(1) << class_id;
  return true;
}

bool h9_static_local_page_debug_take(uint32_t class_id, uint32_t* slot_out) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9StaticLocalClass* cls = &h9_static_local_page_tls.classes[class_id];
  uint64_t bits = cls->local_free_bits;
  if (bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(bits);
  uint64_t bit = UINT64_C(1) << slot;
  cls->local_free_bits &= ~bit;
  cls->local_alloc_bits |= bit;
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

bool h9_static_local_page_debug_free_allocated(uint32_t class_id,
                                               uint32_t slot) {
  if (!h9_static_local_page_class_slot_valid(class_id, slot)) {
    return false;
  }
  H9StaticLocalClass* cls = &h9_static_local_page_tls.classes[class_id];
  uint64_t bit = UINT64_C(1) << slot;
  if ((cls->local_alloc_bits & bit) == 0u ||
      (cls->local_free_bits & bit) != 0u) {
    return false;
  }
  cls->local_alloc_bits &= ~bit;
  cls->local_free_bits |= bit;
  return true;
}

uint64_t h9_static_local_page_debug_free_bits(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return 0;
  }
  return h9_static_local_page_tls.classes[class_id].local_free_bits;
}

uint64_t h9_static_local_page_debug_alloc_bits(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return 0;
  }
  return h9_static_local_page_tls.classes[class_id].local_alloc_bits;
}

uint64_t h9_static_local_page_debug_touched_classes(void) {
  return h9_static_local_page_tls.touched_class_bits;
}

#else
typedef int h9_static_local_page_translation_unit_silence;
#endif
