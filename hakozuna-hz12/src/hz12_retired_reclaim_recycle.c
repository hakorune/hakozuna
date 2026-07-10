#include "hz12_retired_reclaim_recycle.h"

#include "hz12.h"
#include "hz12_reclaim_carve_diag.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_span_decommit.h"
#include "hz12_span_depot.h"
#include "hz12_span_detach.h"
#include "hz12_span_owner_shadow.h"

#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

typedef struct H12RecycleEntry {
  void* span_base;
  uint8_t class_id;
  uint8_t taken;
} H12RecycleEntry;

static int h12_recycle_owner_matches(uint32_t span_id,
                                     H12OwnerToken owner) {
  H12OwnerToken observed;
  return h12_span_owner_shadow_query(span_id, &observed) &&
         observed.slot == owner.slot &&
         observed.generation == owner.generation;
}

static int h12_recycle_is_decommitted(const void* span_base) {
#if defined(_WIN32)
  MEMORY_BASIC_INFORMATION memory;
  return VirtualQuery(span_base, &memory, sizeof(memory)) == sizeof(memory) &&
         memory.State == MEM_RESERVE;
#else
  (void)span_base;
  return 0;
#endif
}

static int h12_recycle_mark_taken(H12RecycleEntry* entries, uint32_t count,
                                  const void* span_base) {
  uint32_t i;
  for (i = 0u; i < count; ++i) {
    if (!entries[i].taken && entries[i].span_base == span_base) {
      entries[i].taken = 1u;
      return 1;
    }
  }
  return 0;
}

static int h12_recycle_exercise_span(
    H12OwnerToken owner, const H12SpanDepotTakeResult* take,
    H12RetiredReclaimRecycleResult* out) {
  H12SpanAccountingRecord accounting;
  H12SpanDetachResult detach;
  H12SpanDecommitResult decommit;
  uint32_t i;
  if (!h12_span_accounting_query(take->span_base, &accounting) ||
      accounting.class_id != take->class_id ||
      !h12_recycle_owner_matches(accounting.span_id, owner)) {
    out->owner_mismatch += 1u;
    return 0;
  }
  for (i = 0u; i < accounting.slot_capacity; ++i) {
    void* ptr = h12_reclaim_carve_current(accounting.class_id);
    uint8_t observed_class;
    if (!ptr || !hz12_span_classify(ptr, &observed_class) ||
        observed_class != accounting.class_id) {
      return 0;
    }
    h12_span_accounting_on_alloc(ptr);
    *((volatile unsigned char*)ptr) = (unsigned char)(i + 1u);
    h12_span_accounting_on_release(ptr);
    hz12_free(ptr);
    out->slots_touched += 1u;
  }
  if (!h12_span_accounting_query(take->span_base, &accounting) ||
      !accounting.accounting_candidate || accounting.live != 0u) {
    return 0;
  }
  if (!h12_span_detach_quiescent(take->span_base, &detach) ||
      !detach.completed) {
    return 0;
  }
  out->second_detach += 1u;
  if (!h12_span_decommit_detached(take->span_base, &decommit) ||
      !decommit.succeeded) {
    return 0;
  }
  out->second_decommit += 1u;
  out->bytes_redecommitted += decommit.bytes;
  return 1;
}

int h12_retired_reclaim_recycle_bounded(
    H12OwnerToken owner, uint32_t budget,
    H12RetiredReclaimRecycleResult* out) {
  H12RecycleEntry entries[HZ12_SPAN_DEPOT_CAP];
  uint32_t span_id;
  uint32_t i;
  if (!out || budget == 0u || budget > HZ12_SPAN_DEPOT_CAP) return 0;
  memset(out, 0, sizeof(*out));
  memset(entries, 0, sizeof(entries));
  out->budget = budget;
  h12_span_depot_reset();

  for (span_id = 0u; span_id < HZ12_SPAN_COUNT && out->initial_put < budget;
       ++span_id) {
    H12SpanAccountingRecord accounting;
    void* span_base;
    if (!h12_recycle_owner_matches(span_id, owner)) continue;
    span_base = hz12_arena_base + ((size_t)span_id << HZ12_SPAN_SHIFT);
    if (!h12_span_accounting_query(span_base, &accounting) ||
        !h12_recycle_is_decommitted(span_base) ||
        !h12_span_depot_put(span_base)) {
      continue;
    }
    entries[out->initial_put].span_base = span_base;
    entries[out->initial_put].class_id = accounting.class_id;
    out->initial_put += 1u;
  }

  for (i = 0u; i < out->initial_put; ++i) {
    H12SpanDepotTakeResult take;
    if (!h12_span_depot_take(entries[i].class_id, &take)) {
      out->failures += 1u;
      break;
    }
    out->take_succeeded += 1u;
    if (!h12_recycle_mark_taken(entries, out->initial_put, take.span_base)) {
      out->address_mismatch += 1u;
      out->failures += 1u;
      break;
    }
    if (!h12_recycle_exercise_span(owner, &take, out)) {
      out->failures += 1u;
      break;
    }
    out->spans_exercised += 1u;
  }

  if (out->failures == 0u) {
    for (i = 0u; i < out->initial_put; ++i) {
      if (!entries[i].taken || !h12_span_depot_put(entries[i].span_base)) {
        out->failures += 1u;
        break;
      }
      out->final_put += 1u;
    }
  }
  out->depot_final = h12_span_depot_count();
  return out->initial_put == budget && out->take_succeeded == budget &&
         out->spans_exercised == budget && out->second_detach == budget &&
         out->second_decommit == budget && out->final_put == budget &&
         out->depot_final == budget && out->address_mismatch == 0u &&
         out->owner_mismatch == 0u && out->failures == 0u &&
         out->bytes_redecommitted == (uint64_t)budget * HZ12_SPAN_BYTES;
}

void h12_retired_reclaim_recycle_dump(
    FILE* out, const H12RetiredReclaimRecycleResult* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_RETIRED_RECLAIM_RECYCLE] budget=%u initial_put=%u "
          "take=%u exercised=%u detach2=%u decommit2=%u final_put=%u "
          "depot_final=%u slots_touched=%llu redecommitted_bytes=%llu "
          "address_mismatch=%u owner_mismatch=%u failures=%u\n",
          result->budget, result->initial_put, result->take_succeeded,
          result->spans_exercised, result->second_detach,
          result->second_decommit, result->final_put, result->depot_final,
          (unsigned long long)result->slots_touched,
          (unsigned long long)result->bytes_redecommitted,
          result->address_mismatch, result->owner_mismatch,
          result->failures);
}
