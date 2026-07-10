#include "hz12_span_decommit.h"

#include "hz12_reclaim_gate.h"
#include "hz12_span.h"

#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

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
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION before;
    MEMORY_BASIC_INFORMATION after;
    if (VirtualQuery(span_base, &before, sizeof(before)) != sizeof(before)) {
      return 0;
    }
    out->before_state = before.State;
    if (before.State != MEM_COMMIT) return 0;
    out->attempted = 1u;
    if (!VirtualFree((LPVOID)span_base, HZ12_SPAN_BYTES, MEM_DECOMMIT)) {
      return 0;
    }
    if (VirtualQuery(span_base, &after, sizeof(after)) != sizeof(after)) {
      return 0;
    }
    out->after_state = after.State;
    out->succeeded = (uint8_t)(after.State == MEM_RESERVE);
  }
#else
  (void)span_base;
#endif
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
