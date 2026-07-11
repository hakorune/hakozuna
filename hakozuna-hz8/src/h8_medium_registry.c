#include "h8_internal.h"
#include "h8_medium.h"
#include "h8_medium_page_shadow.h"

#define H8_MEDIUM_DIRECTORY_CAP 65536u

static h8_platform_mutex_t h8_medium_lock = H8_PLATFORM_MUTEX_INIT;
static H8MediumRun* h8_medium_runs;
static H8MediumRun* h8_medium_detached_by_class[H8_MEDIUM_CLASS_COUNT];
static _Atomic uintptr_t h8_medium_directory_addr;
static _Atomic uintptr_t h8_medium_min_addr;
static _Atomic uintptr_t h8_medium_max_addr;

#if defined(H8_ENABLE_DEBUG_STATS)
static uint64_t h8_medium_registry_now_ns(void) {
  return h8_platform_now_ns();
}
#endif

void h8_medium_lock_global(void) {
#if defined(H8_ENABLE_DEBUG_STATS)
  uint64_t start = h8_medium_registry_now_ns();
#endif
  h8_platform_mutex_lock(&h8_medium_lock);
#if defined(H8_ENABLE_DEBUG_STATS)
  H8_DEBUG_ADD(medium_global_lock_wait_ns,
               (size_t)(h8_medium_registry_now_ns() - start));
#endif
}

void h8_medium_unlock_global(void) {
  h8_platform_mutex_unlock(&h8_medium_lock);
}

H8MediumRun* h8_medium_global_head(void) {
  return h8_medium_runs;
}

H8MediumRun* h8_medium_detached_head_locked(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  return h8_medium_detached_by_class[class_id];
}

void h8_medium_detached_add_locked(H8MediumRun* run) {
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT || run->detached_indexed) {
    return;
  }
  run->next_detached = h8_medium_detached_by_class[run->class_id];
  h8_medium_detached_by_class[run->class_id] = run;
  run->detached_indexed = true;
}

void h8_medium_detached_remove_locked(H8MediumRun* run) {
  if (!run || run->class_id >= H8_MEDIUM_CLASS_COUNT ||
      !run->detached_indexed) {
    return;
  }
  H8MediumRun** cur = &h8_medium_detached_by_class[run->class_id];
  while (*cur) {
    if (*cur == run) {
      *cur = run->next_detached;
      run->next_detached = NULL;
      run->detached_indexed = false;
      return;
    }
    cur = &(*cur)->next_detached;
  }
  run->next_detached = NULL;
  run->detached_indexed = false;
}

static uintptr_t h8_medium_quantum_base(uintptr_t addr) {
  return addr & ~(uintptr_t)(H8_MEDIUM_QUANTUM_BYTES - 1u);
}

static uint64_t h8_medium_hash_quantum(uintptr_t quantum_base) {
  return (uint64_t)(quantum_base / H8_MEDIUM_QUANTUM_BYTES) *
         UINT64_C(11400714819323198485);
}

static inline bool h8_medium_ptr_in_run_fast(const H8MediumRun* run,
                                             const void* ptr) {
  if (!run || !run->base || !ptr) {
    return false;
  }
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t addr = (uintptr_t)ptr;
  size_t payload = (size_t)run->slot_size * (size_t)run->slot_count;
  return addr >= base && addr < base + payload;
}

bool h8_medium_ptr_in_run(const H8MediumRun* run, const void* ptr) {
  return h8_medium_ptr_in_run_fast(run, ptr);
}

static void h8_medium_directory_note_range_locked(H8MediumRun* run) {
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  uintptr_t min =
      atomic_load_explicit(&h8_medium_min_addr, memory_order_relaxed);
  if (min == 0 || base < min) {
    atomic_store_explicit(&h8_medium_min_addr, base, memory_order_release);
  }
  uintptr_t max =
      atomic_load_explicit(&h8_medium_max_addr, memory_order_relaxed);
  if (end > max) {
    atomic_store_explicit(&h8_medium_max_addr, end, memory_order_release);
  }
}

static bool h8_medium_directory_ensure_locked(void) {
  if (atomic_load_explicit(&h8_medium_directory_addr, memory_order_acquire) != 0) {
    return true;
  }
  _Atomic(H8MediumRun*)* directory =
      h8_sys_calloc(H8_MEDIUM_DIRECTORY_CAP, sizeof(*directory));
  if (!directory) {
    return false;
  }
  atomic_store_explicit(&h8_medium_directory_addr, (uintptr_t)directory,
                        memory_order_release);
  return true;
}

static void h8_medium_directory_insert_quantum_locked(H8MediumRun* run,
                                                      uintptr_t quantum_base) {
  _Atomic(H8MediumRun*)* directory = (_Atomic(H8MediumRun*)*)atomic_load_explicit(
      &h8_medium_directory_addr, memory_order_acquire);
  size_t mask = H8_MEDIUM_DIRECTORY_CAP - 1u;
  size_t pos = (size_t)h8_medium_hash_quantum(quantum_base) & mask;
  for (size_t n = 0; n < H8_MEDIUM_DIRECTORY_CAP; ++n) {
    _Atomic(H8MediumRun*)* slot = &directory[(pos + n) & mask];
    H8MediumRun* cur = atomic_load_explicit(slot, memory_order_acquire);
    if (!cur) {
      atomic_store_explicit(slot, run, memory_order_release);
      return;
    }
  }
}

static void h8_medium_directory_insert_locked(H8MediumRun* run) {
  if (!run || !run->base || !h8_medium_directory_ensure_locked()) {
    return;
  }
  h8_medium_directory_note_range_locked(run);
  uintptr_t base = (uintptr_t)run->base;
  uintptr_t end = base + run->run_size;
  for (uintptr_t quantum = base; quantum < end;
       quantum += H8_MEDIUM_QUANTUM_BYTES) {
    h8_medium_directory_insert_quantum_locked(run, quantum);
  }
#if defined(H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0)
  h8_medium_page_shadow_register(run);
#endif
}

static void h8_medium_directory_remove_locked(H8MediumRun* run) {
  _Atomic(H8MediumRun*)* directory = (_Atomic(H8MediumRun*)*)atomic_load_explicit(
      &h8_medium_directory_addr, memory_order_acquire);
  if (!run || !directory) {
    return;
  }
#if defined(H8_MEDIUM_PAGE_SUBSTRATE_SHADOW_L0)
  h8_medium_page_shadow_unregister(run);
#endif
  for (size_t n = 0; n < H8_MEDIUM_DIRECTORY_CAP; ++n) {
    _Atomic(H8MediumRun*)* slot = &directory[n];
    H8MediumRun* cur = atomic_load_explicit(slot, memory_order_acquire);
    if (cur == run) {
      atomic_store_explicit(slot, NULL, memory_order_release);
    }
  }
}

H8MediumRun* h8_medium_directory_find(const void* ptr) {
  _Atomic(H8MediumRun*)* directory = (_Atomic(H8MediumRun*)*)atomic_load_explicit(
      &h8_medium_directory_addr, memory_order_acquire);
  if (!directory || !ptr) {
    return NULL;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t min = atomic_load_explicit(&h8_medium_min_addr, memory_order_acquire);
  uintptr_t max = atomic_load_explicit(&h8_medium_max_addr, memory_order_acquire);
  if (min != 0 && (addr < min || addr >= max)) {
    return NULL;
  }
  uintptr_t quantum = h8_medium_quantum_base(addr);
  size_t mask = H8_MEDIUM_DIRECTORY_CAP - 1u;
  size_t pos = (size_t)h8_medium_hash_quantum(quantum) & mask;
  for (size_t n = 0; n < H8_MEDIUM_DIRECTORY_CAP; ++n) {
    H8MediumRun* run = atomic_load_explicit(&directory[(pos + n) & mask],
                                            memory_order_acquire);
    if (run && h8_medium_ptr_in_run_fast(run, ptr)) {
      return run;
    }
  }
  return NULL;
}

H8MediumRun* h8_medium_find_run_locked(const void* ptr, bool route_lookup) {
  for (H8MediumRun* run = h8_medium_runs; run; run = run->next_global) {
    if (route_lookup) {
      H8_DEBUG_INC(medium_route_lookup_step_count);
    } else {
      H8_DEBUG_INC(medium_free_lookup_step_count);
    }
    if (h8_medium_ptr_in_run_fast(run, ptr)) {
      return run;
    }
  }
  return NULL;
}

void h8_medium_register_locked(H8MediumRun* run) {
  run->next_global = h8_medium_runs;
  h8_medium_runs = run;
  h8_medium_directory_insert_locked(run);
}

void h8_medium_unregister_locked(H8MediumRun* run) {
  h8_medium_detached_remove_locked(run);
  H8MediumRun** cur = &h8_medium_runs;
  while (*cur) {
    if (*cur == run) {
      *cur = run->next_global;
      run->next_global = NULL;
      h8_medium_directory_remove_locked(run);
      return;
    }
    cur = &(*cur)->next_global;
  }
}
