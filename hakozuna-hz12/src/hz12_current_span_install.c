#include "hz12_current_span_install.h"

int h12_current_span_install(H12ThreadCache* cache, uint8_t class_id,
                             void* span_base) {
#if HZ12_CLASSIFY_SPAN
  H12SpanCurrent* current;
  size_t slot_bytes;
  if (!cache || !span_base || class_id >= HZ12_CLASS_COUNT) return 0;
  current = &cache->current[class_id];
  if (current->base != NULL) return 0;
  slot_bytes = hz12_class_slot_size(class_id);
  if (slot_bytes == 0u) return 0;
  current->base = (char*)span_base;
  current->bump_index = 0u;
  current->slot_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  return 1;
#else
  (void)cache;
  (void)class_id;
  (void)span_base;
  return 0;
#endif
}
