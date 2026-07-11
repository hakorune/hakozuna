#include "hz12_span_decommit.h"

#include "hz12_reclaim_gate.h"
#include "hz12_span.h"
#include "hz12_span_backing.h"

#include <string.h>

int h12_span_decommit_detached(const void* ptr,
                               H12SpanDecommitResult* out) {
  H12ReclaimGate gate;
  const void* span_base;
  if (!out) return 0;
  memset(out, 0, sizeof(*out));
  if (!h12_reclaim_gate_query(ptr, 1, &gate) || !gate.gate_open) return 0;
  out->span_id = gate.span_id;
  out->bytes = HZ12_SPAN_BYTES;
  out->gate_open = 1u;
  span_base = hz12_arena_base + ((size_t)gate.span_id << HZ12_SPAN_SHIFT);
  out->attempted = 1u;
  out->succeeded = (uint8_t)h12_span_backing_discard(
      (void*)span_base, &out->before_state, &out->after_state);
  return out->succeeded != 0u;
}

void h12_span_decommit_dump(FILE* out,
                            const H12SpanDecommitResult* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_SPAN_DECOMMIT] span=%u bytes=%u gate_open=%u attempted=%u "
          "succeeded=%u before_state=0x%x after_state=0x%x\n",
          result->span_id, result->bytes, (unsigned)result->gate_open,
          (unsigned)result->attempted, (unsigned)result->succeeded,
          result->before_state, result->after_state);
}
