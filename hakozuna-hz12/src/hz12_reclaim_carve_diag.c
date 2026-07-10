#include "hz12_reclaim_carve_diag.h"

#include "hz12_thread_cache.h"

void* h12_reclaim_carve_current(uint8_t class_id) {
#if HZ12_CLASSIFY_SPAN
  H12ThreadCache* cache = hz12_tls;
  H12SpanCurrent* current;
  size_t slot_bytes;
  if (!cache || class_id >= HZ12_CLASS_COUNT) return NULL;
  current = &cache->current[class_id];
  slot_bytes = hz12_class_slot_size(class_id);
  if (!current->base || slot_bytes == 0u ||
      current->bump_index >= current->slot_count) {
    return NULL;
  }
  return current->base + (size_t)current->bump_index++ * slot_bytes;
#else
  (void)class_id;
  return NULL;
#endif
}
