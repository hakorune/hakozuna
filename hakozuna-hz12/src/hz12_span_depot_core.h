#ifndef HZ12_SPAN_DEPOT_CORE_H
#define HZ12_SPAN_DEPOT_CORE_H

#include <stdint.h>

#ifndef HZ12_SPAN_DEPOT_CAP
#define HZ12_SPAN_DEPOT_CAP 64u
#endif

typedef struct H12SpanDepotCoreEntry {
  void* span_base;
  uint8_t class_id;
} H12SpanDepotCoreEntry;

void h12_span_depot_core_reset(void);
uint32_t h12_span_depot_core_reserve(uint32_t requested);
void h12_span_depot_core_release_reservation(uint32_t count);
int h12_span_depot_core_put_reserved(void* span_base, uint8_t class_id);
int h12_span_depot_core_put_decommitted(void* span_base, uint8_t class_id);
int h12_span_depot_core_take(uint8_t class_id, H12SpanDepotCoreEntry* out);
uint32_t h12_span_depot_core_count(void);
uint32_t h12_span_depot_core_max(void);
uint32_t h12_span_depot_core_available(void);

#endif /* HZ12_SPAN_DEPOT_CORE_H */
