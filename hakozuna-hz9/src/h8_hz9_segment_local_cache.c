#include "h8_internal.h"

#if defined(H9_SEGMENT_LOCAL_CACHE_L0)

#include <string.h>

typedef enum H9SegmentState {
  H9_SEGMENT_LOCAL = 0,
  H9_SEGMENT_REMOTE_SEEN = 1,
  H9_SEGMENT_RETIRED = 2
} H9SegmentState;

typedef struct H9SegmentLocalClass {
  uintptr_t base_addr;
  uint32_t slot_size;
  uint32_t run_size;
  size_t payload_bytes;
  uint16_t slot_count;
  uint64_t local_free_bits;
  uint64_t local_alloc_bits;
  uint64_t remote_pending_bits;
  H9SegmentState state;
} H9SegmentLocalClass;

typedef struct H9SegmentLocalState {
  H9SegmentLocalClass classes[H8_MEDIUM_CLASS_COUNT];
  uint64_t touched_class_bits;
  uint32_t active_class;
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

static H8RouteKind h9_segment_route_cached_offset_to_slot(
    const H9SegmentLocalClass* cls, size_t offset, bool fast,
    uint32_t* slot_out) {
  if (!cls || cls->slot_size == 0u || cls->slot_count == 0u) {
    return H8_ROUTE_MISS;
  }
  if (offset >= (size_t)cls->run_size) {
    return H8_ROUTE_MISS;
  }
  if (offset >= cls->payload_bytes) {
    return H8_ROUTE_INVALID;
  }
  if (fast && (cls->slot_size & (cls->slot_size - 1u)) == 0u) {
    if ((offset & ((size_t)cls->slot_size - 1u)) != 0u) {
      return H8_ROUTE_INVALID;
    }
    if (slot_out) {
      *slot_out = (uint32_t)(offset >> __builtin_ctz(cls->slot_size));
    }
    return H8_ROUTE_VALID;
  }
  if (fast && cls->slot_count == 2u) {
    if (offset == 0u || offset == (size_t)cls->slot_size) {
      if (slot_out) {
        *slot_out = offset == 0u ? 0u : 1u;
      }
      return H8_ROUTE_VALID;
    }
    return H8_ROUTE_INVALID;
  }
  if ((offset % (size_t)cls->slot_size) != 0u) {
    return H8_ROUTE_INVALID;
  }
  if (slot_out) {
    *slot_out = (uint32_t)(offset / (size_t)cls->slot_size);
  }
  return H8_ROUTE_VALID;
}

static H8RouteKind h9_segment_route_offset_to_slot(uint32_t class_id,
                                                   size_t offset,
                                                   uint32_t* slot_out) {
  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  if (!h9_segment_local_cache_debug_class_geometry(
          class_id, &slot_size, &run_size, &slot_count)) {
    return H8_ROUTE_MISS;
  }
  size_t payload_bytes = (size_t)slot_size * (size_t)slot_count;
  if (offset >= (size_t)run_size) {
    return H8_ROUTE_MISS;
  }
  if (offset >= payload_bytes || (offset % (size_t)slot_size) != 0u) {
    return H8_ROUTE_INVALID;
  }
  if (slot_out) {
    *slot_out = (uint32_t)(offset / (size_t)slot_size);
  }
  return H8_ROUTE_VALID;
}

void h9_segment_local_cache_debug_reset(void) {
  memset(&h9_segment_tls, 0, sizeof(h9_segment_tls));
  h9_segment_tls.active_class = UINT32_MAX;
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

bool h9_segment_local_cache_debug_cycle_known(uint32_t class_id,
                                              uintptr_t* addr_out) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_LOCAL ||
      cls->local_free_bits == 0u || cls->base_addr == 0u ||
      cls->slot_size == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(cls->local_free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  cls->local_free_bits &= ~bit;
  cls->local_alloc_bits |= bit;
  if (addr_out) {
    *addr_out = cls->base_addr + (uintptr_t)slot * (uintptr_t)cls->slot_size;
  }
  cls->local_alloc_bits &= ~bit;
  cls->local_free_bits |= bit;
  return true;
}

bool h9_segment_local_cache_debug_active_cycle_known(uintptr_t* addr_out) {
  if (h9_segment_tls.active_class >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  return h9_segment_local_cache_debug_cycle_known(h9_segment_tls.active_class,
                                                  addr_out);
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

H8RouteKind h9_segment_local_cache_debug_route_offset(uint32_t class_id,
                                                      size_t offset) {
  return h9_segment_route_offset_to_slot(class_id, offset, NULL);
}

bool h9_segment_local_cache_debug_bind_base(uint32_t class_id,
                                            uintptr_t base_addr) {
  uint32_t slot_size = 0u;
  uint32_t run_size = 0u;
  uint16_t slot_count = 0u;
  if (base_addr == 0u ||
      !h9_segment_local_cache_debug_class_geometry(
          class_id, &slot_size, &run_size, &slot_count) ||
      base_addr > UINTPTR_MAX - (uintptr_t)run_size) {
    return false;
  }
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_LOCAL ||
      cls->local_free_bits != 0u || cls->local_alloc_bits != 0u ||
      cls->remote_pending_bits != 0u) {
    return false;
  }
  cls->base_addr = base_addr;
  cls->slot_size = slot_size;
  cls->run_size = run_size;
  cls->slot_count = slot_count;
  cls->payload_bytes = (size_t)slot_size * (size_t)slot_count;
  h9_segment_tls.touched_class_bits |= UINT64_C(1) << class_id;
  return true;
}

H8RouteKind h9_segment_local_cache_debug_route_addr(uint32_t class_id,
                                                    uintptr_t addr) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || addr < cls->base_addr) {
    return H8_ROUTE_MISS;
  }
  uintptr_t offset = addr - cls->base_addr;
  if (offset >= (uintptr_t)cls->run_size) {
    return H8_ROUTE_MISS;
  }
  if (cls->state != H9_SEGMENT_LOCAL) {
    return H8_ROUTE_INVALID;
  }
  return h9_segment_route_cached_offset_to_slot(cls, (size_t)offset, false,
                                                NULL);
}

H8RouteKind h9_segment_local_cache_debug_route_table_addr(uintptr_t addr,
                                                          uint32_t* class_out) {
  for (uint32_t class_id = 0u; class_id < H8_MEDIUM_CLASS_COUNT; ++class_id) {
    H9SegmentLocalClass* cls = h9_segment_class(class_id);
    if (!cls || cls->base_addr == 0u || addr < cls->base_addr) {
      continue;
    }
    uintptr_t offset = addr - cls->base_addr;
    if (offset >= (uintptr_t)cls->run_size) {
      continue;
    }
    if (class_out) {
      *class_out = class_id;
    }
    return h9_segment_local_cache_debug_route_addr(class_id, addr);
  }
  return H8_ROUTE_MISS;
}

H8RouteKind h9_segment_local_cache_debug_route_table_slot_addr(
    uintptr_t addr,
    uint32_t* class_out,
    uint32_t* slot_out) {
  for (uint32_t class_id = 0u; class_id < H8_MEDIUM_CLASS_COUNT; ++class_id) {
    H9SegmentLocalClass* cls = h9_segment_class(class_id);
    if (!cls || cls->base_addr == 0u || addr < cls->base_addr) {
      continue;
    }
    uintptr_t offset = addr - cls->base_addr;
    if (offset >= (uintptr_t)cls->run_size) {
      continue;
    }
    if (class_out) {
      *class_out = class_id;
    }
    if (cls->state != H9_SEGMENT_LOCAL) {
      return H8_ROUTE_INVALID;
    }
    return h9_segment_route_cached_offset_to_slot(cls, (size_t)offset, true,
                                                  slot_out);
  }
  return H8_ROUTE_MISS;
}

H8RouteKind h9_segment_local_cache_debug_route_active_slot_addr(
    uintptr_t addr,
    uint32_t* class_out,
    uint32_t* slot_out) {
  uint32_t class_id = h9_segment_tls.active_class;
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (cls && cls->base_addr != 0u && addr >= cls->base_addr) {
    uintptr_t offset = addr - cls->base_addr;
    if (offset < (uintptr_t)cls->run_size) {
      if (class_out) {
        *class_out = class_id;
      }
      if (cls->state != H9_SEGMENT_LOCAL) {
        return H8_ROUTE_INVALID;
      }
      return h9_segment_route_cached_offset_to_slot(cls, (size_t)offset, true,
                                                    slot_out);
    }
  }
  return h9_segment_local_cache_debug_route_table_slot_addr(addr, class_out,
                                                            slot_out);
}

H8RouteKind h9_segment_local_cache_debug_route_active_slot_only_addr(
    uintptr_t addr,
    uint32_t* class_out,
    uint32_t* slot_out) {
  uint32_t class_id = h9_segment_tls.active_class;
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || addr < cls->base_addr) {
    return H8_ROUTE_MISS;
  }
  uintptr_t offset = addr - cls->base_addr;
  if (offset >= (uintptr_t)cls->run_size) {
    return H8_ROUTE_MISS;
  }
  if (class_out) {
    *class_out = class_id;
  }
  if (cls->state != H9_SEGMENT_LOCAL) {
    return H8_ROUTE_INVALID;
  }
  return h9_segment_route_cached_offset_to_slot(cls, (size_t)offset, true,
                                                slot_out);
}

H8RouteKind h9_segment_local_cache_debug_route_active_range_addr(
    uintptr_t addr,
    uint32_t* class_out) {
  uint32_t class_id = h9_segment_tls.active_class;
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || addr < cls->base_addr) {
    return H8_ROUTE_MISS;
  }
  uintptr_t offset = addr - cls->base_addr;
  if (offset >= (uintptr_t)cls->run_size) {
    return H8_ROUTE_MISS;
  }
  if (class_out) {
    *class_out = class_id;
  }
  if (cls->state != H9_SEGMENT_LOCAL || offset >= cls->payload_bytes) {
    return H8_ROUTE_INVALID;
  }
  return H8_ROUTE_VALID;
}

bool h9_segment_local_cache_debug_set_active_class(uint32_t class_id) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || cls->state != H9_SEGMENT_LOCAL) {
    return false;
  }
  h9_segment_tls.active_class = class_id;
  return true;
}

bool h9_segment_local_cache_debug_take_slot_addr(uint32_t class_id,
                                                 uint32_t* slot_out,
                                                 uintptr_t* addr_out) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || cls->slot_size == 0u) {
    return false;
  }
  uint32_t slot = 0u;
  if (!h9_segment_local_cache_debug_take(class_id, &slot)) {
    return false;
  }
  if (slot_out) {
    *slot_out = slot;
  }
  if (addr_out) {
    *addr_out = cls->base_addr + (uintptr_t)slot * (uintptr_t)cls->slot_size;
  }
  return true;
}

bool h9_segment_local_cache_debug_active_take_direct(uint32_t* slot_out,
                                                     uintptr_t* addr_out) {
  uint32_t class_id = h9_segment_tls.active_class;
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->state != H9_SEGMENT_LOCAL ||
      cls->local_free_bits == 0u || cls->base_addr == 0u ||
      cls->slot_size == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(cls->local_free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  cls->local_free_bits &= ~bit;
  cls->local_alloc_bits |= bit;
  if (slot_out) {
    *slot_out = slot;
  }
  if (addr_out) {
    *addr_out = cls->base_addr + (uintptr_t)slot * (uintptr_t)cls->slot_size;
  }
  return true;
}

bool h9_segment_local_cache_debug_take_addr(uint32_t class_id,
                                            uintptr_t* addr_out) {
  return h9_segment_local_cache_debug_take_slot_addr(class_id, NULL, addr_out);
}

bool h9_segment_local_cache_debug_free_addr(uint32_t class_id,
                                            uintptr_t addr) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || addr < cls->base_addr ||
      cls->state != H9_SEGMENT_LOCAL) {
    return false;
  }
  uint32_t slot = 0u;
  uintptr_t offset = addr - cls->base_addr;
  if (h9_segment_route_cached_offset_to_slot(cls, (size_t)offset, false,
                                             &slot) != H8_ROUTE_VALID) {
    return false;
  }
  return h9_segment_local_cache_debug_free_allocated(class_id, slot);
}

bool h9_segment_local_cache_debug_free_addr_fast(uint32_t class_id,
                                                 uintptr_t addr) {
  H9SegmentLocalClass* cls = h9_segment_class(class_id);
  if (!cls || cls->base_addr == 0u || addr < cls->base_addr ||
      cls->state != H9_SEGMENT_LOCAL) {
    return false;
  }
  uint32_t slot = 0u;
  uintptr_t offset = addr - cls->base_addr;
  if (h9_segment_route_cached_offset_to_slot(cls, (size_t)offset, true,
                                             &slot) != H8_ROUTE_VALID) {
    return false;
  }
  return h9_segment_local_cache_debug_free_allocated(class_id, slot);
}

bool h9_segment_local_cache_debug_active_take_addr(uintptr_t* addr_out) {
  if (h9_segment_tls.active_class >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  return h9_segment_local_cache_debug_take_addr(h9_segment_tls.active_class,
                                                addr_out);
}

bool h9_segment_local_cache_debug_active_free_addr(uintptr_t addr) {
  if (h9_segment_tls.active_class >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  return h9_segment_local_cache_debug_free_addr(h9_segment_tls.active_class,
                                                addr);
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
