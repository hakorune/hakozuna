#ifndef H8_SLOT_GEOMETRY_INLINE_H
#define H8_SLOT_GEOMETRY_INLINE_H

static inline bool h8_slot_index_from_ptr_checked(const H8Span* span,
                                                  const void* ptr,
                                                  size_t* slot_out) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = (uintptr_t)span->base;
  uintptr_t offset = addr - base;
#if defined(H8_CLASS_MAP_UPPER3072)
  uint32_t class_id = span->class_id;
  if (H8_LIKELY(class_id <= 7u)) {
    uint32_t shift = 4u + class_id;
    uintptr_t mask = ((uintptr_t)1 << shift) - (uintptr_t)1;
    if ((offset & mask) != 0) {
      return false;
    }
    size_t slot = (size_t)(offset >> shift);
    if (slot >= span->slot_count) {
      return false;
    }
    *slot_out = slot;
    return true;
  }
  if (class_id == 8u) {
    if ((offset & 1023u) != 0) {
      return false;
    }
    uintptr_t quantum = offset >> 10u;
    if ((quantum % 3u) != 0) {
      return false;
    }
    size_t slot = (size_t)(quantum / 3u);
    if (slot >= span->slot_count) {
      return false;
    }
    *slot_out = slot;
    return true;
  }
  if ((offset & 4095u) != 0) {
    return false;
  }
  size_t slot = (size_t)(offset >> 12u);
#else
  uint32_t shift = h8_class_shift(span->class_id);
  uintptr_t mask = ((uintptr_t)1 << shift) - (uintptr_t)1;
  if ((offset & mask) != 0) {
    return false;
  }
  uintptr_t quantum = offset >> shift;
  uint32_t factor = h8_class_factor(span->class_id);
  size_t slot = (size_t)quantum;
  if (factor == 3u) {
    if ((quantum % 3u) != 0) {
      return false;
    }
    slot = (size_t)(quantum / 3u);
  }
#endif
  if (slot >= span->slot_count) {
    return false;
  }
  *slot_out = slot;
  return true;
}

static inline void* h8_slot_ptr(const H8Span* span, size_t slot) {
#if defined(H8_CLASS_MAP_UPPER3072)
  uint32_t class_id = span->class_id;
  if (H8_LIKELY(class_id <= 7u)) {
    return span->base + (slot << (4u + class_id));
  }
  if (class_id == 8u) {
    return span->base + ((slot * 3u) << 10u);
  }
  return span->base + (slot << 12u);
#else
  uint32_t factor = h8_class_factor(span->class_id);
  uint32_t shift = h8_class_shift(span->class_id);
  if (factor == 1u) {
    return span->base + (slot << shift);
  }
  return span->base + ((slot * factor) << shift);
#endif
}

#endif
