#include "hz12_span_depot.h"

#include "hz12_port.h"
#include "hz12_span.h"
#include "hz12_span_accounting.h"
#include "hz12_thread_cache.h"

#include <stdatomic.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

typedef struct H12SpanDepotEntry {
  void* span_base;
  uint8_t class_id;
} H12SpanDepotEntry;

static H12SpanDepotEntry h12_depot_entries[HZ12_SPAN_DEPOT_CAP];
static HZ12_MUTEX h12_depot_lock;
static uint32_t h12_depot_count;
static uint32_t h12_depot_max;
static int h12_depot_initialized;
static _Atomic uint64_t h12_put_attempt;
static _Atomic uint64_t h12_put_success;
static _Atomic uint64_t h12_put_duplicate;
static _Atomic uint64_t h12_put_full;
static _Atomic uint64_t h12_put_reject_state;
static _Atomic uint64_t h12_take_attempt;
static _Atomic uint64_t h12_take_hit;
static _Atomic uint64_t h12_take_miss;
static _Atomic uint64_t h12_recommit_success;
static _Atomic uint64_t h12_recommit_fail;
static _Atomic uint64_t h12_route_fail;
static _Atomic uint64_t h12_current_install_fail;
static _Atomic uint64_t h12_rollback;

static int h12_depot_push_raw(void* span_base, uint8_t class_id) {
  uint32_t i;
  int inserted = 0;
  hz12_mutex_lock(&h12_depot_lock);
  for (i = 0u; i < h12_depot_count; ++i) {
    if (h12_depot_entries[i].span_base == span_base) {
      hz12_mutex_unlock(&h12_depot_lock);
      return -1;
    }
  }
  if (h12_depot_count < HZ12_SPAN_DEPOT_CAP) {
    h12_depot_entries[h12_depot_count].span_base = span_base;
    h12_depot_entries[h12_depot_count].class_id = class_id;
    h12_depot_count += 1u;
    if (h12_depot_count > h12_depot_max) h12_depot_max = h12_depot_count;
    inserted = 1;
  }
  hz12_mutex_unlock(&h12_depot_lock);
  return inserted;
}

static int h12_depot_pop_class(uint8_t class_id,
                               H12SpanDepotEntry* out) {
  uint32_t i;
  int found = 0;
  hz12_mutex_lock(&h12_depot_lock);
  for (i = h12_depot_count; i > 0u; --i) {
    if (h12_depot_entries[i - 1u].class_id == class_id) {
      *out = h12_depot_entries[i - 1u];
      h12_depot_entries[i - 1u] = h12_depot_entries[h12_depot_count - 1u];
      h12_depot_count -= 1u;
      found = 1;
      break;
    }
  }
  hz12_mutex_unlock(&h12_depot_lock);
  return found;
}

static int h12_depot_install_current(void* span_base, uint8_t class_id) {
#if HZ12_CLASSIFY_SPAN
  H12ThreadCache* cache = hz12_thread_cache_get();
  H12SpanCurrent* current;
  size_t slot_bytes;
  if (!cache || class_id >= HZ12_CLASS_COUNT) return 0;
  current = &cache->current[class_id];
  if (current->base != NULL) return 0;
  slot_bytes = hz12_class_slot_size(class_id);
  if (slot_bytes == 0u) return 0;
  current->base = (char*)span_base;
  current->bump_index = 0u;
  current->slot_count = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  return 1;
#else
  (void)span_base;
  (void)class_id;
  return 0;
#endif
}

void h12_span_depot_reset(void) {
  if (!h12_depot_initialized) {
    hz12_mutex_init(&h12_depot_lock);
    h12_depot_initialized = 1;
  }
  hz12_mutex_lock(&h12_depot_lock);
  memset(h12_depot_entries, 0, sizeof(h12_depot_entries));
  h12_depot_count = 0u;
  h12_depot_max = 0u;
  hz12_mutex_unlock(&h12_depot_lock);
  atomic_store_explicit(&h12_put_attempt, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_put_success, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_put_duplicate, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_put_full, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_put_reject_state, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_take_attempt, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_take_hit, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_take_miss, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_recommit_success, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_recommit_fail, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_route_fail, 0u, memory_order_relaxed);
  atomic_store_explicit(&h12_current_install_fail, 0u,
                        memory_order_relaxed);
  atomic_store_explicit(&h12_rollback, 0u, memory_order_relaxed);
}

int h12_span_depot_put(const void* ptr) {
  H12SpanAccountingRecord accounting;
  void* span_base;
  int result;
  atomic_fetch_add_explicit(&h12_put_attempt, 1u, memory_order_relaxed);
  if (!h12_depot_initialized ||
      !h12_span_accounting_query(ptr, &accounting)) {
    atomic_fetch_add_explicit(&h12_put_reject_state, 1u,
                              memory_order_relaxed);
    return 0;
  }
  span_base = hz12_arena_base +
      ((size_t)accounting.span_id << HZ12_SPAN_SHIFT);
  if (hz12_span_class[accounting.span_id] != 0u) {
    atomic_fetch_add_explicit(&h12_put_reject_state, 1u,
                              memory_order_relaxed);
    return 0;
  }
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION memory;
    if (VirtualQuery(span_base, &memory, sizeof(memory)) != sizeof(memory) ||
        memory.State != MEM_RESERVE) {
      atomic_fetch_add_explicit(&h12_put_reject_state, 1u,
                                memory_order_relaxed);
      return 0;
    }
  }
#endif
  result = h12_depot_push_raw(span_base, accounting.class_id);
  if (result > 0) {
    atomic_fetch_add_explicit(&h12_put_success, 1u, memory_order_relaxed);
    return 1;
  }
  atomic_fetch_add_explicit(result < 0 ? &h12_put_duplicate : &h12_put_full,
                            1u, memory_order_relaxed);
  return 0;
}

int h12_span_depot_take(uint8_t class_id, H12SpanDepotTakeResult* out) {
  H12SpanDepotEntry entry;
  if (!out || !h12_depot_initialized || class_id >= HZ12_CLASS_COUNT) return 0;
  atomic_fetch_add_explicit(&h12_take_attempt, 1u, memory_order_relaxed);
  memset(out, 0, sizeof(*out));
  if (!h12_depot_pop_class(class_id, &entry)) {
    atomic_fetch_add_explicit(&h12_take_miss, 1u, memory_order_relaxed);
    return 0;
  }
  atomic_fetch_add_explicit(&h12_take_hit, 1u, memory_order_relaxed);
  out->span_base = entry.span_base;
  out->class_id = entry.class_id;
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION before;
    MEMORY_BASIC_INFORMATION after;
    void* committed;
    if (VirtualQuery(entry.span_base, &before, sizeof(before)) !=
            sizeof(before) ||
        before.State != MEM_RESERVE) {
      (void)h12_depot_push_raw(entry.span_base, entry.class_id);
      atomic_fetch_add_explicit(&h12_recommit_fail, 1u,
                                memory_order_relaxed);
      return 0;
    }
    out->before_state = before.State;
    committed = VirtualAlloc(entry.span_base, HZ12_SPAN_BYTES, MEM_COMMIT,
                             PAGE_READWRITE);
    if (committed != entry.span_base) {
      (void)h12_depot_push_raw(entry.span_base, entry.class_id);
      atomic_fetch_add_explicit(&h12_recommit_fail, 1u,
                                memory_order_relaxed);
      return 0;
    }
    out->recommitted = 1u;
    atomic_fetch_add_explicit(&h12_recommit_success, 1u,
                              memory_order_relaxed);
    if (VirtualQuery(entry.span_base, &after, sizeof(after)) != sizeof(after)) {
      goto rollback;
    }
    out->after_state = after.State;
    if (after.State != MEM_COMMIT) goto rollback;
  }
#else
  (void)h12_depot_push_raw(entry.span_base, entry.class_id);
  return 0;
#endif
  out->accounting_reset = (uint8_t)h12_span_accounting_recycle(
      entry.span_base, entry.class_id);
  if (!out->accounting_reset) goto rollback;
  out->route_attached = (uint8_t)hz12_span_route_attach(
      entry.span_base, entry.class_id);
  if (!out->route_attached) {
    atomic_fetch_add_explicit(&h12_route_fail, 1u, memory_order_relaxed);
    goto rollback;
  }
  out->current_installed = (uint8_t)h12_depot_install_current(
      entry.span_base, entry.class_id);
  if (!out->current_installed) {
    atomic_fetch_add_explicit(&h12_current_install_fail, 1u,
                              memory_order_relaxed);
    goto rollback;
  }
  return 1;

rollback:
  atomic_fetch_add_explicit(&h12_rollback, 1u, memory_order_relaxed);
  if (out->route_attached) {
    (void)hz12_span_route_detach(entry.span_base, entry.class_id);
    out->route_attached = 0u;
  }
#if defined(_WIN32)
  if (out->recommitted) {
    (void)VirtualFree(entry.span_base, HZ12_SPAN_BYTES, MEM_DECOMMIT);
    out->recommitted = 0u;
  }
#endif
  (void)h12_depot_push_raw(entry.span_base, entry.class_id);
  return 0;
}

uint32_t h12_span_depot_count(void) {
  uint32_t count;
  if (!h12_depot_initialized) return 0u;
  hz12_mutex_lock(&h12_depot_lock);
  count = h12_depot_count;
  hz12_mutex_unlock(&h12_depot_lock);
  return count;
}

void h12_span_depot_stats(H12SpanDepotStats* out) {
  if (!out) return;
  memset(out, 0, sizeof(*out));
  out->put_attempt = atomic_load_explicit(&h12_put_attempt,
                                          memory_order_relaxed);
  out->put_success = atomic_load_explicit(&h12_put_success,
                                          memory_order_relaxed);
  out->put_duplicate = atomic_load_explicit(&h12_put_duplicate,
                                            memory_order_relaxed);
  out->put_full = atomic_load_explicit(&h12_put_full, memory_order_relaxed);
  out->put_reject_state = atomic_load_explicit(&h12_put_reject_state,
                                               memory_order_relaxed);
  out->take_attempt = atomic_load_explicit(&h12_take_attempt,
                                           memory_order_relaxed);
  out->take_hit = atomic_load_explicit(&h12_take_hit, memory_order_relaxed);
  out->take_miss = atomic_load_explicit(&h12_take_miss, memory_order_relaxed);
  out->recommit_success = atomic_load_explicit(&h12_recommit_success,
                                               memory_order_relaxed);
  out->recommit_fail = atomic_load_explicit(&h12_recommit_fail,
                                            memory_order_relaxed);
  out->route_fail = atomic_load_explicit(&h12_route_fail,
                                         memory_order_relaxed);
  out->current_install_fail = atomic_load_explicit(&h12_current_install_fail,
                                                   memory_order_relaxed);
  out->rollback = atomic_load_explicit(&h12_rollback, memory_order_relaxed);
  hz12_mutex_lock(&h12_depot_lock);
  out->depot_current = h12_depot_count;
  out->depot_max = h12_depot_max;
  hz12_mutex_unlock(&h12_depot_lock);
}

void h12_span_depot_dump(FILE* out, const H12SpanDepotTakeResult* result) {
  H12SpanDepotStats stats;
  if (!out || !result) return;
  h12_span_depot_stats(&stats);
  fprintf(out,
          "[HZ12_SPAN_DEPOT] class=%u count=%u recommitted=%u "
          "accounting_reset=%u route_attached=%u current_installed=%u "
          "before_state=0x%x after_state=0x%x\n",
          (unsigned)result->class_id, h12_span_depot_count(),
          (unsigned)result->recommitted,
          (unsigned)result->accounting_reset,
          (unsigned)result->route_attached,
          (unsigned)result->current_installed, result->before_state,
          result->after_state);
  fprintf(out,
          "[HZ12_SPAN_DEPOT_STATS] put_attempt=%llu put_success=%llu "
          "put_duplicate=%llu put_full=%llu put_reject_state=%llu "
          "take_attempt=%llu take_hit=%llu take_miss=%llu "
          "recommit_success=%llu recommit_fail=%llu route_fail=%llu "
          "current_install_fail=%llu rollback=%llu current=%u max=%u\n",
          (unsigned long long)stats.put_attempt,
          (unsigned long long)stats.put_success,
          (unsigned long long)stats.put_duplicate,
          (unsigned long long)stats.put_full,
          (unsigned long long)stats.put_reject_state,
          (unsigned long long)stats.take_attempt,
          (unsigned long long)stats.take_hit,
          (unsigned long long)stats.take_miss,
          (unsigned long long)stats.recommit_success,
          (unsigned long long)stats.recommit_fail,
          (unsigned long long)stats.route_fail,
          (unsigned long long)stats.current_install_fail,
          (unsigned long long)stats.rollback, stats.depot_current,
          stats.depot_max);
}
