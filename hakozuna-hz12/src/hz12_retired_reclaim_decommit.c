#include "hz12_retired_reclaim_decommit.h"

#include "hz12_reclaim_gate.h"
#include "hz12_span.h"
#include "hz12_span_decommit.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

int h12_retired_reclaim_decommit_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimDecommitResult* out) {
  uint32_t span_id;
  if (!out || budget == 0u) return 0;
  memset(out, 0, sizeof(*out));
  out->budget = budget;
  for (span_id = 0u; span_id < HZ12_SPAN_COUNT && out->attempts < budget;
       ++span_id) {
    H12OwnerToken observed;
    H12ReclaimGate gate;
    H12SpanDecommitResult decommit;
    void* span_base;
    if (!h12_span_owner_shadow_query(span_id, &observed) ||
        observed.slot != owner.slot ||
        observed.generation != owner.generation) {
      continue;
    }
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!h12_reclaim_gate_query(span_base, 1, &gate) || !gate.gate_open) {
      continue;
    }
    out->candidates += 1u;
    out->attempts += 1u;
    if (h12_span_decommit_detached(span_base, &decommit) &&
        decommit.succeeded) {
      out->succeeded += 1u;
      out->decommitted_bytes += decommit.bytes;
    } else {
      out->failed += 1u;
      break;
    }
  }
  return out->failed == 0u && out->succeeded != 0u;
}

void h12_retired_reclaim_decommit_dump(
    FILE* out, const H12RetiredReclaimDecommitResult* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_RETIRED_RECLAIM_DECOMMIT] budget=%u candidates=%u "
          "attempts=%u succeeded=%u failed=%u decommitted_bytes=%llu\n",
          result->budget, result->candidates, result->attempts,
          result->succeeded, result->failed,
          (unsigned long long)result->decommitted_bytes);
}
