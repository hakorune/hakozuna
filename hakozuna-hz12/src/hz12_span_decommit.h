#ifndef HZ12_SPAN_DECOMMIT_H
#define HZ12_SPAN_DECOMMIT_H

#include <stdint.h>
#include <stdio.h>

typedef struct H12SpanDecommitResult {
  uint32_t span_id;
  uint32_t bytes;
  uint32_t before_state;
  uint32_t after_state;
  uint8_t gate_open;
  uint8_t attempted;
  uint8_t succeeded;
} H12SpanDecommitResult;

int h12_span_decommit_detached(const void* ptr,
                               H12SpanDecommitResult* out);
void h12_span_decommit_dump(FILE* out,
                            const H12SpanDecommitResult* result);

#endif /* HZ12_SPAN_DECOMMIT_H */
