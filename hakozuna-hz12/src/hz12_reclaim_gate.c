#include "hz12_reclaim_gate.h"

#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_thread_cache.h"

#include <string.h>

static int h12_gate_ptr_in_span(const void* ptr, const void* span_base) {
  const uintptr_t value = (uintptr_t)ptr;
  const uintptr_t begin = (uintptr_t)span_base;
  return ptr && value >= begin && value < begin + HZ12_SPAN_BYTES;
}

static uint32_t h12_gate_current_refs(const H12ThreadCache* cache,
                                      uint8_t class_id,
                                      const void* span_base) {
#if HZ12_CLASSIFY_SPAN
  if (cache && class_id < HZ12_CLASS_COUNT &&
      cache->current[class_id].base == span_base) {
    return 1u;
  }
#else
  (void)cache;
  (void)class_id;
  (void)span_base;
#endif
  return 0u;
}

static uint32_t h12_gate_front_objects(const H12ThreadCache* cache,
                                       uint8_t class_id,
                                       const void* span_base) {
  uint32_t count = 0u;
  uint32_t i;
  uint32_t item_count;
  if (!cache || class_id >= HZ12_CLASS_COUNT) return 0u;
#if HZ12_CACHE_SOA
  item_count = cache->class_counts[class_id];
  for (i = 0u; i < item_count; ++i) {
    if (h12_gate_ptr_in_span(cache->class_items[class_id][i], span_base)) {
      count += 1u;
    }
  }
#else
  {
    const H12ClassCache* class_cache = &cache->class_cache[class_id];
#if HZ12_CACHE_TOPPTR
    item_count = (uint32_t)(class_cache->top - class_cache->items);
#else
    item_count = class_cache->count;
#endif
    for (i = 0u; i < item_count; ++i) {
      if (h12_gate_ptr_in_span(class_cache->items[i], span_base)) {
        count += 1u;
      }
    }
  }
#endif
  return count;
}

int h12_reclaim_gate_query(const void* ptr, int thread_scope_complete,
                           H12ReclaimGate* out) {
  H12SpanAccountingRecord accounting;
  H12WholeSpanShadow global;
  const void* span_base;
  if (!out || !h12_span_accounting_query(ptr, &accounting)) return 0;
  memset(out, 0, sizeof(*out));
  span_base = hz12_arena_base +
      ((size_t)accounting.span_id << HZ12_SPAN_SHIFT);
  global = h12_span_accounting_scan();
  out->span_id = accounting.span_id;
  out->class_id = accounting.class_id;
  out->slot_capacity = accounting.slot_capacity;
  out->accounting_candidate = accounting.accounting_candidate;
  out->thread_scope_complete = (uint8_t)(thread_scope_complete != 0);
  out->accounting_clean =
      (uint8_t)(global.release_untracked == 0u &&
                global.release_underflow == 0u);
  out->current_span_refs = h12_gate_current_refs(
      hz12_tls, accounting.class_id, span_base);
  out->front_cache_objects = h12_gate_front_objects(
      hz12_tls, accounting.class_id, span_base);
  out->returned_sink_objects = hz12_returned_count_in_span(
      accounting.class_id, span_base);
  out->route_attached = (uint8_t)(hz12_span_class[accounting.span_id] != 0u);
  out->gate_open =
      (uint8_t)(out->accounting_candidate && out->accounting_clean &&
                out->thread_scope_complete && out->current_span_refs == 0u &&
                out->front_cache_objects == 0u &&
                out->returned_sink_objects == 0u && !out->route_attached);
  return 1;
}

void h12_reclaim_gate_dump(FILE* out, const H12ReclaimGate* gate) {
  if (!out || !gate) return;
  fprintf(out,
          "[HZ12_RECLAIM_GATE] span=%u class=%u slots=%u accounting=%u "
          "accounting_clean=%u thread_scope_complete=%u current_refs=%u "
          "front_cache=%u returned_sink=%u route_attached=%u gate_open=%u\n",
          gate->span_id, (unsigned)gate->class_id, gate->slot_capacity,
          (unsigned)gate->accounting_candidate,
          (unsigned)gate->accounting_clean,
          (unsigned)gate->thread_scope_complete, gate->current_span_refs,
          gate->front_cache_objects, gate->returned_sink_objects,
          (unsigned)gate->route_attached, (unsigned)gate->gate_open);
}
