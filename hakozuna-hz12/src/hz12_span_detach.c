#include "hz12_span_detach.h"

#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_thread_cache.h"

#include <string.h>

static int h12_detach_ptr_in_span(const void* ptr, const void* span_base) {
  const uintptr_t value = (uintptr_t)ptr;
  const uintptr_t begin = (uintptr_t)span_base;
  return ptr && value >= begin && value < begin + HZ12_SPAN_BYTES;
}

static uint32_t h12_detach_front_cache(H12ThreadCache* cache,
                                       uint8_t class_id,
                                       const void* span_base) {
  uint32_t read_index;
  uint32_t write_index = 0u;
  uint32_t item_count;
  uint32_t removed;
  size_t removed_bytes;
  if (!cache || class_id >= HZ12_CLASS_COUNT) return 0u;
#if HZ12_CACHE_SOA
  item_count = cache->class_counts[class_id];
  for (read_index = 0u; read_index < item_count; ++read_index) {
    void* ptr = cache->class_items[class_id][read_index];
    if (!h12_detach_ptr_in_span(ptr, span_base)) {
      cache->class_items[class_id][write_index++] = ptr;
    }
  }
  for (read_index = write_index; read_index < item_count; ++read_index) {
    cache->class_items[class_id][read_index] = NULL;
  }
  cache->class_counts[class_id] = write_index;
#else
  {
    H12ClassCache* class_cache = &cache->class_cache[class_id];
#if HZ12_CACHE_TOPPTR
    item_count = (uint32_t)(class_cache->top - class_cache->items);
#else
    item_count = class_cache->count;
#endif
    for (read_index = 0u; read_index < item_count; ++read_index) {
      void* ptr = class_cache->items[read_index];
      if (!h12_detach_ptr_in_span(ptr, span_base)) {
        class_cache->items[write_index++] = ptr;
      }
    }
    for (read_index = write_index; read_index < item_count; ++read_index) {
      class_cache->items[read_index] = NULL;
    }
#if HZ12_CACHE_TOPPTR
    class_cache->top = class_cache->items + write_index;
#else
    class_cache->count = write_index;
#endif
  }
#endif
  removed = item_count - write_index;
#if HZ12_CACHE_BYTE_ACCOUNTING
  removed_bytes = hz12_class_slot_size(class_id) * (size_t)removed;
  if (removed_bytes <= cache->cached_bytes) {
    cache->cached_bytes -= removed_bytes;
  } else {
    cache->cached_bytes = 0u;
  }
#else
  (void)removed_bytes;
#endif
  return removed;
}

static uint8_t h12_detach_current_span(H12ThreadCache* cache,
                                       uint8_t class_id,
                                       const void* span_base) {
#if HZ12_CLASSIFY_SPAN
  H12SpanCurrent* current;
  if (!cache || class_id >= HZ12_CLASS_COUNT) return 0u;
  current = &cache->current[class_id];
  if (current->base != span_base) return 0u;
  current->base = NULL;
  current->bump_index = 0u;
  current->slot_count = 0u;
  return 1u;
#else
  (void)cache;
  (void)class_id;
  (void)span_base;
  return 0u;
#endif
}

int h12_span_detach_quiescent(const void* ptr, H12SpanDetachResult* out) {
  H12SpanAccountingRecord accounting;
  const void* span_base;
  uint32_t detached_objects;
  if (!out || !h12_span_accounting_query(ptr, &accounting)) return 0;
  memset(out, 0, sizeof(*out));
  if (!h12_reclaim_gate_query(ptr, 1, &out->before)) return 0;
  if (!out->before.accounting_candidate || !out->before.accounting_clean ||
      !out->before.thread_scope_complete || !out->before.route_attached ||
      out->before.front_cache_objects + out->before.returned_sink_objects !=
          accounting.slot_capacity) {
    return 0;
  }
  span_base = hz12_arena_base +
      ((size_t)accounting.span_id << HZ12_SPAN_SHIFT);
  out->front_cache_detached = h12_detach_front_cache(
      hz12_tls, accounting.class_id, span_base);
  out->returned_sink_detached = hz12_returned_detach_span(
      accounting.class_id, span_base);
  out->current_span_detached = h12_detach_current_span(
      hz12_tls, accounting.class_id, span_base);
  detached_objects = out->front_cache_detached +
                     out->returned_sink_detached;
  if (detached_objects != accounting.slot_capacity) return 0;
  if (!h12_reclaim_gate_query(ptr, 1, &out->after)) return 0;
  if (out->after.current_span_refs != 0u ||
      out->after.front_cache_objects != 0u ||
      out->after.returned_sink_objects != 0u || !out->after.route_attached) {
    return 0;
  }
  out->route_detached = (uint8_t)hz12_span_route_detach(
      span_base, accounting.class_id);
  if (!out->route_detached ||
      !h12_reclaim_gate_query(ptr, 1, &out->after)) {
    return 0;
  }
  out->completed = out->after.gate_open;
  return out->completed != 0u;
}

void h12_span_detach_dump(FILE* out, const H12SpanDetachResult* result) {
  if (!out || !result) return;
  fprintf(out,
          "[HZ12_SPAN_DETACH] front=%u returned=%u current=%u route=%u "
          "completed=%u\n",
          result->front_cache_detached, result->returned_sink_detached,
          (unsigned)result->current_span_detached,
          (unsigned)result->route_detached, (unsigned)result->completed);
  h12_reclaim_gate_dump(out, &result->after);
}
