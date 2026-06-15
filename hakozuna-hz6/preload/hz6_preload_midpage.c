#include "hz6_preload_midpage.h"

#include "hz6_allocator_api_front.h"
#include "hz6_midpage_front.h"
#include "hz6_preload_stats.h"

#include <stdint.h>

#if HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1 || \
    HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1
static __thread uintptr_t
    g_hz6_preload_midpage_page_hint
        [HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY];

static size_t hz6_preload_midpage_page_hint_slot(uintptr_t page) {
  uintptr_t mixed = page ^ (page >> 17) ^ (page >> 29);
  return (size_t)(mixed %
                  HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY);
}

static void hz6_preload_midpage_page_hint_note_one(uintptr_t page) {
  uintptr_t key = page + 1u;
  size_t slot = hz6_preload_midpage_page_hint_slot(page);
  for (size_t probe = 0;
       probe < HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_PROBE_LIMIT; ++probe) {
    uintptr_t* entry =
        &g_hz6_preload_midpage_page_hint
             [(slot + probe) % HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY];
    if (*entry == 0 || *entry == key) {
      *entry = key;
      return;
    }
  }
  g_hz6_preload_midpage_page_hint[slot] = key;
}

HZ6_PRELOAD_MIDPAGE_INTERNAL void
hz6_preload_midpage_hint_note(const void* ptr, size_t size) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base_page = addr >> 12;
  uintptr_t last_page = (addr + size - 1u) >> 12;
  hz6_preload_midpage_page_hint_note_one(base_page);
  if (last_page != base_page) {
    hz6_preload_midpage_page_hint_note_one(last_page);
  }
}

HZ6_PRELOAD_MIDPAGE_INTERNAL int
hz6_preload_midpage_hint_maybe(const void* ptr) {
  if (!ptr) {
    return 0;
  }
  uintptr_t page = ((uintptr_t)ptr) >> 12;
  uintptr_t key = page + 1u;
  size_t slot = hz6_preload_midpage_page_hint_slot(page);
  for (size_t probe = 0;
       probe < HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_PROBE_LIMIT; ++probe) {
    uintptr_t entry =
        g_hz6_preload_midpage_page_hint
            [(slot + probe) % HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY];
    if (entry == key) {
      return 1;
    }
    if (entry == 0) {
      return 0;
    }
  }
  return 0;
}
#elif HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1
static __thread uintptr_t g_hz6_preload_midpage_hint_min;
static __thread uintptr_t g_hz6_preload_midpage_hint_max;

HZ6_PRELOAD_MIDPAGE_INTERNAL void
hz6_preload_midpage_hint_note(const void* ptr, size_t size) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t min = addr & ~((uintptr_t)4095u);
  uintptr_t max = (addr + size - 1u) | (uintptr_t)4095u;
  if (g_hz6_preload_midpage_hint_min == 0 ||
      min < g_hz6_preload_midpage_hint_min) {
    g_hz6_preload_midpage_hint_min = min;
  }
  if (max > g_hz6_preload_midpage_hint_max) {
    g_hz6_preload_midpage_hint_max = max;
  }
}

HZ6_PRELOAD_MIDPAGE_INTERNAL int
hz6_preload_midpage_hint_maybe(const void* ptr) {
  if (!ptr || g_hz6_preload_midpage_hint_min == 0) {
    return 0;
  }
  uintptr_t addr = (uintptr_t)ptr;
  return addr >= g_hz6_preload_midpage_hint_min &&
         addr <= g_hz6_preload_midpage_hint_max;
}
#endif

#if HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_NOINLINE __attribute__((noinline))
#else
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_NOINLINE
#endif

HZ6_PRELOAD_MIDPAGE_INTERNAL HZ6_PRELOAD_MIDPAGE_BOUNDARY_NOINLINE void*
hz6_preload_malloc_midpage_boundary(Hz6Allocator* allocator, size_t size) {
  hz6_preload_phase_count(
      &g_hz6_preload_phase_stats.malloc_midpage_boundary_attempt);
#if HZ6_PRELOAD_MIDPAGE_BOUNDARY_FUSED_L1 && \
    HZ6_PRELOAD_MIDPAGE_DIRECT_CLASS_L1
  uint16_t class_id = size <= HZ6_MIDPAGE_8K_BYTES
                          ? HZ6_MIDPAGE_8K_CLASS_ID
                          : HZ6_MIDPAGE_32K_CLASS_ID;
  void* ptr = hz6_allocator_preload_midpage_malloc_class_skip_transfer(
      allocator, class_id, size);
#else
  void* ptr =
      hz6_allocator_preload_midpage_malloc_skip_transfer(allocator, size);
#endif
  if (ptr) {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_midpage_boundary_hit);
#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
    hz6_preload_midpage_hint_note(ptr, size);
#endif
  } else {
    hz6_preload_phase_count(
        &g_hz6_preload_phase_stats.malloc_midpage_boundary_fallback);
  }
  return ptr;
}
#endif
