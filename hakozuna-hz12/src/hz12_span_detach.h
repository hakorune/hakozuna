#ifndef HZ12_SPAN_DETACH_H
#define HZ12_SPAN_DETACH_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_reclaim_gate.h"

typedef struct H12SpanDetachResult {
  H12ReclaimGate before;
  H12ReclaimGate after;
  uint32_t front_cache_detached;
  uint32_t returned_sink_detached;
  uint8_t current_span_detached;
  uint8_t route_detached;
  uint8_t completed;
} H12SpanDetachResult;

int h12_span_detach_quiescent(const void* ptr, H12SpanDetachResult* out);
void h12_span_detach_dump(FILE* out, const H12SpanDetachResult* result);

#endif /* HZ12_SPAN_DETACH_H */
