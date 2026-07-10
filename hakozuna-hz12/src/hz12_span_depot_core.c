#include "hz12_span_depot_core.h"

#include "hz12_port.h"
#include "hz12_span.h"

#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static H12SpanDepotCoreEntry h12_depot_entries[HZ12_SPAN_DEPOT_CAP];
static HZ12_MUTEX h12_depot_lock;
static uint32_t h12_depot_count;
static uint32_t h12_depot_max;
static int h12_depot_initialized;

void h12_span_depot_core_reset(void) {
  if (!h12_depot_initialized) {
    hz12_mutex_init(&h12_depot_lock);
    h12_depot_initialized = 1;
  }
  hz12_mutex_lock(&h12_depot_lock);
  memset(h12_depot_entries, 0, sizeof(h12_depot_entries));
  h12_depot_count = 0u;
  h12_depot_max = 0u;
  hz12_mutex_unlock(&h12_depot_lock);
}

int h12_span_depot_core_put_decommitted(void* span_base, uint8_t class_id) {
  uint32_t i;
  int inserted = 0;
  if (!h12_depot_initialized || !span_base ||
      class_id >= HZ12_CLASS_COUNT || !hz12_arena_contains(span_base) ||
      (((uintptr_t)span_base - (uintptr_t)hz12_arena_base) &
       (HZ12_SPAN_BYTES - 1u)) != 0u) {
    return 0;
  }
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION memory;
    if (VirtualQuery(span_base, &memory, sizeof(memory)) != sizeof(memory) ||
        memory.State != MEM_RESERVE) {
      return 0;
    }
  }
#endif
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

int h12_span_depot_core_take(uint8_t class_id, H12SpanDepotCoreEntry* out) {
  uint32_t i;
  int found = 0;
  if (!out || !h12_depot_initialized || class_id >= HZ12_CLASS_COUNT) return 0;
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

uint32_t h12_span_depot_core_count(void) {
  uint32_t count;
  if (!h12_depot_initialized) return 0u;
  hz12_mutex_lock(&h12_depot_lock);
  count = h12_depot_count;
  hz12_mutex_unlock(&h12_depot_lock);
  return count;
}

uint32_t h12_span_depot_core_max(void) {
  uint32_t maximum;
  if (!h12_depot_initialized) return 0u;
  hz12_mutex_lock(&h12_depot_lock);
  maximum = h12_depot_max;
  hz12_mutex_unlock(&h12_depot_lock);
  return maximum;
}
