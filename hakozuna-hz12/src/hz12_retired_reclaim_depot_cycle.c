#include "hz12_retired_reclaim_depot_cycle.h"

#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_depot.h"
#include "hz12_span_owner_shadow.h"
#include "hz12_thread_cache.h"

#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

typedef struct H12DepotCycleEntry {
  void* span_base;
  uint8_t class_id;
  uint8_t consumed;
} H12DepotCycleEntry;

static int h12_depot_cycle_owner_matches(uint32_t span_id,
                                         H12OwnerToken owner) {
  H12OwnerToken observed;
  return h12_span_owner_shadow_query(span_id, &observed) &&
         observed.slot == owner.slot &&
         observed.generation == owner.generation;
}

static int h12_depot_cycle_consume(H12DepotCycleEntry* entries,
                                   uint32_t count, const void* span_base) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (!entries[i].consumed && entries[i].span_base == span_base) {
      entries[i].consumed = 1u;
      return 1;
    }
  }
  return 0;
}

static int h12_depot_cycle_is_decommitted(const void* span_base) {
#if defined(_WIN32)
  MEMORY_BASIC_INFORMATION memory;
  return VirtualQuery(span_base, &memory, sizeof(memory)) == sizeof(memory) &&
         memory.State == MEM_RESERVE;
#else
  (void)span_base;
  return 0;
#endif
}

int h12_retired_reclaim_depot_cycle_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimDepotCycleResult* out) {
  H12DepotCycleEntry entries[HZ12_SPAN_DEPOT_CAP];
  uint32_t span_id;
  uint32_t i;
  if (!out || budget == 0u || budget > HZ12_SPAN_DEPOT_CAP) return 0;
  memset(out, 0, sizeof(*out));
  memset(entries, 0, sizeof(entries));
  out->budget = budget;

  for (span_id = 0u;
       span_id < HZ12_SPAN_COUNT && out->put_succeeded < budget; ++span_id) {
    H12SpanAccountingRecord accounting;
    void* span_base;
    if (!h12_depot_cycle_owner_matches(span_id, owner)) continue;
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!h12_span_accounting_query(span_base, &accounting)) continue;
    if (!h12_depot_cycle_is_decommitted(span_base)) continue;
    out->candidates += 1u;
    out->put_attempts += 1u;
    if (!h12_span_depot_put(span_base)) continue;
    entries[out->put_succeeded].span_base = span_base;
    entries[out->put_succeeded].class_id = accounting.class_id;
    out->put_succeeded += 1u;
  }

  for (i = 0u; i < out->put_succeeded; ++i) {
    H12SpanDepotTakeResult take;
    H12SpanAccountingRecord accounting;
    out->take_attempts += 1u;
    if (!h12_span_depot_take(entries[i].class_id, &take)) break;
    out->take_succeeded += 1u;
    if (!h12_depot_cycle_consume(entries, out->put_succeeded,
                                 take.span_base)) {
      out->address_mismatch += 1u;
    }
    if (!h12_span_accounting_query(take.span_base, &accounting) ||
        !h12_depot_cycle_owner_matches(accounting.span_id, owner)) {
      out->owner_mismatch += 1u;
    }
    if (take.recommitted && take.accounting_reset && take.route_attached &&
        take.current_installed) {
      out->recommitted_bytes += HZ12_SPAN_BYTES;
    }
    hz12_thread_cache_reclaim_checkpoint();
    out->checkpoint_count += 1u;
  }
  out->depot_remaining = h12_span_depot_count();
  return out->put_succeeded == budget &&
         out->take_succeeded == out->put_succeeded &&
         out->address_mismatch == 0u && out->owner_mismatch == 0u &&
         out->checkpoint_count == out->take_succeeded &&
         out->depot_remaining == 0u &&
         out->recommitted_bytes == (uint64_t)budget * HZ12_SPAN_BYTES;
}

void h12_retired_reclaim_depot_cycle_dump(
    FILE* out, const H12RetiredReclaimDepotCycleResult* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_RETIRED_RECLAIM_DEPOT_CYCLE] budget=%u candidates=%u "
          "put=%u/%u take=%u/%u address_mismatch=%u owner_mismatch=%u "
          "checkpoints=%u depot_remaining=%u recommitted_bytes=%llu\n",
          result->budget, result->candidates, result->put_succeeded,
          result->put_attempts, result->take_succeeded,
          result->take_attempts, result->address_mismatch,
          result->owner_mismatch, result->checkpoint_count,
          result->depot_remaining,
          (unsigned long long)result->recommitted_bytes);
}
