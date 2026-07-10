#ifndef HZ12_RECLAIM_GATE_H
#define HZ12_RECLAIM_GATE_H

#include <stdint.h>
#include <stdio.h>

typedef struct H12ReclaimGate {
  uint32_t span_id;
  uint32_t slot_capacity;
  uint32_t current_span_refs;
  uint32_t front_cache_objects;
  uint32_t returned_sink_objects;
  uint8_t class_id;
  uint8_t accounting_candidate;
  uint8_t route_attached;
  uint8_t thread_scope_complete;
  uint8_t accounting_clean;
  uint8_t gate_open;
} H12ReclaimGate;

int h12_reclaim_gate_query(const void* ptr, int thread_scope_complete,
                           H12ReclaimGate* out);
void h12_reclaim_gate_dump(FILE* out, const H12ReclaimGate* gate);

#endif /* HZ12_RECLAIM_GATE_H */
