#include "hz12_span.h"
#include "hz12_port.h"
#include "hz12_class_diag.h"

#include <stdatomic.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

char* hz12_arena_base = NULL;
uint8_t hz12_span_class[HZ12_SPAN_COUNT]; /* BSS zero-fill == uncarved */
static _Atomic uint64_t hz12_span_create_count;
#ifndef HZ12_SPAN_RETURNED_DIAG
#define HZ12_SPAN_RETURNED_DIAG 0
#endif
#if HZ12_SPAN_RETURNED_DIAG
static _Atomic uint64_t hz12_returned_push_count;
static _Atomic uint64_t hz12_returned_pop_hit_count;
static _Atomic uint64_t hz12_returned_pop_miss_count;
#endif

static char* hz12_arena_end = NULL;
static _Atomic(char*) hz12_span_cursor;
#if defined(_WIN32)
static INIT_ONCE hz12_span_init_once = INIT_ONCE_STATIC_INIT;
#else
static pthread_once_t hz12_span_init_once = PTHREAD_ONCE_INIT;
#endif

/* Per-class returned-object sink. */
typedef struct H12Returned {
  HZ12_MUTEX lock;
  void* head;
} H12Returned;
static H12Returned hz12_returned[HZ12_CLASS_COUNT];

static void hz12_span_init_real(void) {
#if defined(_WIN32)
  void* base = VirtualAlloc(NULL, (SIZE_T)HZ12_ARENA_BYTES,
                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!base) {
    hz12_arena_base = NULL;
    hz12_arena_end = NULL;
    return;
  }
#else
  void* base = mmap(NULL, HZ12_ARENA_BYTES, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (base == MAP_FAILED) {
    hz12_arena_base = NULL;
    hz12_arena_end = NULL;
    return;
  }
#if HZ12_ARENA_NOHUGEPAGE
  (void)madvise(base, HZ12_ARENA_BYTES, MADV_NOHUGEPAGE);
#endif
#endif
  hz12_arena_base = (char*)base;
  hz12_arena_end = hz12_arena_base + HZ12_ARENA_BYTES;
  atomic_store_explicit(&hz12_span_cursor, hz12_arena_base, memory_order_relaxed);
  for (uint32_t i = 0; i < HZ12_CLASS_COUNT; ++i) {
    hz12_mutex_init(&hz12_returned[i].lock);
    hz12_returned[i].head = NULL;
  }
}

#if defined(_WIN32)
static BOOL CALLBACK hz12_span_init_once_callback(PINIT_ONCE init_once,
                                                  PVOID parameter,
                                                  PVOID* context) {
  (void)init_once;
  (void)parameter;
  (void)context;
  hz12_span_init_real();
  return TRUE;
}
#endif

void hz12_span_init(void) {
#if defined(_WIN32)
  (void)InitOnceExecuteOnce(&hz12_span_init_once,
                            hz12_span_init_once_callback,
                            NULL,
                            NULL);
#else
  pthread_once(&hz12_span_init_once, hz12_span_init_real);
#endif
}

void* hz12_span_carve_for_class(uint8_t class_id) {
  hz12_span_init();
  if (!hz12_arena_base) {
    return NULL;
  }
  for (;;) {
    char* cur = atomic_load_explicit(&hz12_span_cursor, memory_order_relaxed);
    if (cur + HZ12_SPAN_BYTES > hz12_arena_end) {
      return NULL; /* arena full */
    }
    char* nxt = cur + HZ12_SPAN_BYTES;
    if (atomic_compare_exchange_weak_explicit(&hz12_span_cursor, &cur, nxt,
                                              memory_order_acq_rel,
                                              memory_order_relaxed)) {
      size_t id = ((size_t)(cur - hz12_arena_base)) >> HZ12_SPAN_SHIFT;
      hz12_span_class[id] = (uint8_t)(class_id + 1u);
      atomic_fetch_add_explicit(&hz12_span_create_count, 1u,
                                memory_order_relaxed);
      return cur;
    }
  }
}

uint64_t hz12_span_create_count_load(void) {
  return atomic_load_explicit(&hz12_span_create_count, memory_order_relaxed);
}

uint64_t hz12_returned_push_count_load(void) {
#if HZ12_SPAN_RETURNED_DIAG
  return atomic_load_explicit(&hz12_returned_push_count, memory_order_relaxed);
#else
  return 0u;
#endif
}

uint64_t hz12_returned_pop_hit_count_load(void) {
#if HZ12_SPAN_RETURNED_DIAG
  return atomic_load_explicit(&hz12_returned_pop_hit_count, memory_order_relaxed);
#else
  return 0u;
#endif
}

uint64_t hz12_returned_pop_miss_count_load(void) {
#if HZ12_SPAN_RETURNED_DIAG
  return atomic_load_explicit(&hz12_returned_pop_miss_count, memory_order_relaxed);
#else
  return 0u;
#endif
}

void hz12_returned_push(uint8_t class_id, void* ptr) {
  if (class_id >= HZ12_CLASS_COUNT || !ptr) {
    return;
  }
  H12Returned* r = &hz12_returned[class_id];
  hz12_mutex_lock(&r->lock);
  *(void**)ptr = r->head;
  r->head = ptr;
  hz12_mutex_unlock(&r->lock);
  HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(class_id, 1);
#if HZ12_SPAN_RETURNED_DIAG
  atomic_fetch_add_explicit(&hz12_returned_push_count, 1u,
                            memory_order_relaxed);
#endif
}

static uint32_t hz12_returned_push_chain(uint8_t class_id,
                                         void** items,
                                         uint32_t count) {
  void* head = NULL;
  void* tail = NULL;
  uint32_t linked_count = 0u;
  /* Build privately; the sink lock covers only the O(1) head splice. */
  for (uint32_t i = 0u; i < count; ++i) {
    if (!items[i]) {
      continue;
    }
    *(void**)items[i] = head;
    if (!tail) {
      tail = items[i];
    }
    head = items[i];
    ++linked_count;
  }
  if (!head) {
    return 0u;
  }
  H12Returned* r = &hz12_returned[class_id];
  hz12_mutex_lock(&r->lock);
  *(void**)tail = r->head;
  r->head = head;
  hz12_mutex_unlock(&r->lock);
  HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(class_id, (int32_t)linked_count);
  HZ12_CLASS_DIAG_RETURNED_PUSH_RANGE(class_id, linked_count);
#if HZ12_SPAN_RETURNED_DIAG
  atomic_fetch_add_explicit(&hz12_returned_push_count, linked_count,
                            memory_order_relaxed);
#endif
  return linked_count;
}

void hz12_returned_push_range(uint8_t class_id, void** items, uint32_t count) {
  if (class_id >= HZ12_CLASS_COUNT || !items || count == 0u) {
    return;
  }
#if HZ12_RETURNED_PUSH_RANGE_CHUNK
  for (uint32_t offset = 0u; offset < count;) {
    uint32_t chunk = count - offset;
    if (chunk > HZ12_RETURNED_PUSH_RANGE_CHUNK) {
      chunk = HZ12_RETURNED_PUSH_RANGE_CHUNK;
    }
    (void)hz12_returned_push_chain(class_id, items + offset, chunk);
    offset += chunk;
  }
#else
  (void)hz12_returned_push_chain(class_id, items, count);
#endif
}

void* hz12_returned_pop(uint8_t class_id) {
  if (class_id >= HZ12_CLASS_COUNT) {
#if HZ12_SPAN_RETURNED_DIAG
    atomic_fetch_add_explicit(&hz12_returned_pop_miss_count, 1u,
                              memory_order_relaxed);
#endif
    return NULL;
  }
  H12Returned* r = &hz12_returned[class_id];
  hz12_mutex_lock(&r->lock);
  void* p = r->head;
  if (p) {
    r->head = *(void**)p;
  }
  hz12_mutex_unlock(&r->lock);
  if (p) {
    HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(class_id, -1);
  }
#if HZ12_SPAN_RETURNED_DIAG
  atomic_fetch_add_explicit(p ? &hz12_returned_pop_hit_count
                              : &hz12_returned_pop_miss_count,
                            1u,
                            memory_order_relaxed);
#endif
  if (p) {
    HZ12_CLASS_DIAG_RETURNED_POP_HIT(class_id);
  } else {
    HZ12_CLASS_DIAG_RETURNED_POP_MISS(class_id);
  }
  return p;
}

uint32_t hz12_returned_pop_range(uint8_t class_id, void** out, uint32_t max) {
  if (class_id >= HZ12_CLASS_COUNT || !out || max == 0u) {
    return 0u;
  }
  H12Returned* r = &hz12_returned[class_id];
  hz12_mutex_lock(&r->lock);
  uint32_t n = 0u;
  while (n < max && r->head) {
    void* p = r->head;
    r->head = *(void**)p;
    out[n++] = p;
  }
  hz12_mutex_unlock(&r->lock);
  if (n > 0u) {
    HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(class_id, -(int32_t)n);
  }
#if HZ12_SPAN_RETURNED_DIAG
  if (n > 0u) {
    atomic_fetch_add_explicit(&hz12_returned_pop_hit_count, n,
                              memory_order_relaxed);
  } else {
    atomic_fetch_add_explicit(&hz12_returned_pop_miss_count, 1u,
                              memory_order_relaxed);
  }
#endif
#if HZ12_CLASS_DIAG
  if (n > 0u) {
    for (uint32_t i = 0u; i < n; ++i) {
      HZ12_CLASS_DIAG_RETURNED_POP_HIT(class_id);
    }
  } else {
    HZ12_CLASS_DIAG_RETURNED_POP_MISS(class_id);
  }
#endif
  return n;
}

uint32_t hz12_returned_count_in_span(uint8_t class_id,
                                     const void* span_base) {
  const uintptr_t begin = (uintptr_t)span_base;
  const uintptr_t end = begin + HZ12_SPAN_BYTES;
  uint32_t count = 0u;
  void* current;
  H12Returned* returned;
  if (class_id >= HZ12_CLASS_COUNT || !span_base) return 0u;
  returned = &hz12_returned[class_id];
  hz12_mutex_lock(&returned->lock);
  current = returned->head;
  while (current) {
    const uintptr_t value = (uintptr_t)current;
    if (value >= begin && value < end) count += 1u;
    current = *(void**)current;
  }
  hz12_mutex_unlock(&returned->lock);
  return count;
}

int hz12_returned_snapshot_span(uint8_t class_id, const void* span_base,
                                H12ReturnedSpanSnapshot* out) {
  uint64_t slots[HZ12_SPAN_BYTES / HZ12_MIN_SIZE / 64u];
  const uintptr_t begin = (uintptr_t)span_base;
  const uintptr_t end = begin + HZ12_SPAN_BYTES;
  const size_t slot_bytes = hz12_class_slot_size(class_id);
  H12Returned* returned;
  void* current;
  if (!out || class_id >= HZ12_CLASS_COUNT || !span_base || slot_bytes == 0u) {
    return 0;
  }
  memset(out, 0, sizeof(*out));
  memset(slots, 0, sizeof(slots));
  out->slot_capacity = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  returned = &hz12_returned[class_id];
  hz12_mutex_lock(&returned->lock);
  current = returned->head;
  while (current) {
    const uintptr_t value = (uintptr_t)current;
    void* next = *(void**)current;
    if (value >= begin && value < end) {
      const uintptr_t offset = value - begin;
      uint32_t slot;
      uint32_t word;
      uint64_t mask;
      out->objects += 1u;
      if ((offset % slot_bytes) != 0u) {
        out->invalid_slots += 1u;
        current = next;
        continue;
      }
      slot = (uint32_t)(offset / slot_bytes);
      if (slot >= out->slot_capacity) {
        out->invalid_slots += 1u;
        current = next;
        continue;
      }
      word = slot / 64u;
      mask = UINT64_C(1) << (slot % 64u);
      if ((slots[word] & mask) != 0u) {
        out->duplicate_slots += 1u;
      } else {
        slots[word] |= mask;
        out->unique_slots += 1u;
      }
    }
    current = next;
  }
  hz12_mutex_unlock(&returned->lock);
  out->complete = (uint8_t)(out->unique_slots == out->slot_capacity &&
      out->objects == out->slot_capacity && out->duplicate_slots == 0u &&
      out->invalid_slots == 0u);
  return 1;
}

void hz12_returned_lock_probe(uint8_t class_id) {
  H12Returned* returned;
  if (class_id >= HZ12_CLASS_COUNT) return;
  returned = &hz12_returned[class_id];
  hz12_mutex_lock(&returned->lock);
  hz12_mutex_unlock(&returned->lock);
}

int hz12_returned_detach_complete_span(uint8_t class_id,
                                       const void* span_base,
                                       H12ReturnedSpanSnapshot* out) {
  uint64_t slots[HZ12_SPAN_BYTES / HZ12_MIN_SIZE / 64u];
  const uintptr_t begin = (uintptr_t)span_base;
  const uintptr_t end = begin + HZ12_SPAN_BYTES;
  const size_t slot_bytes = hz12_class_slot_size(class_id);
  H12Returned* returned;
  void* current;
  void** link;
  if (!out || class_id >= HZ12_CLASS_COUNT || !span_base || slot_bytes == 0u) {
    return 0;
  }
  memset(out, 0, sizeof(*out));
  memset(slots, 0, sizeof(slots));
  out->slot_capacity = (uint32_t)(HZ12_SPAN_BYTES / slot_bytes);
  returned = &hz12_returned[class_id];
  hz12_mutex_lock(&returned->lock);
  current = returned->head;
  while (current) {
    const uintptr_t value = (uintptr_t)current;
    void* next = *(void**)current;
    if (value >= begin && value < end) {
      const uintptr_t offset = value - begin;
      uint32_t slot;
      uint32_t word;
      uint64_t mask;
      out->objects += 1u;
      if ((offset % slot_bytes) != 0u) {
        out->invalid_slots += 1u;
        current = next;
        continue;
      }
      slot = (uint32_t)(offset / slot_bytes);
      if (slot >= out->slot_capacity) {
        out->invalid_slots += 1u;
        current = next;
        continue;
      }
      word = slot / 64u;
      mask = UINT64_C(1) << (slot % 64u);
      if ((slots[word] & mask) != 0u) {
        out->duplicate_slots += 1u;
      } else {
        slots[word] |= mask;
        out->unique_slots += 1u;
      }
    }
    current = next;
  }
  out->complete = (uint8_t)(out->unique_slots == out->slot_capacity &&
      out->objects == out->slot_capacity && out->duplicate_slots == 0u &&
      out->invalid_slots == 0u);
  if (!out->complete) {
    hz12_mutex_unlock(&returned->lock);
    return 0;
  }
  link = &returned->head;
  while (*link) {
    current = *link;
    if ((uintptr_t)current >= begin && (uintptr_t)current < end) {
      *link = *(void**)current;
    } else {
      link = (void**)current;
    }
  }
  hz12_mutex_unlock(&returned->lock);
  return 1;
}

uint32_t hz12_returned_detach_span(uint8_t class_id,
                                   const void* span_base) {
  const uintptr_t begin = (uintptr_t)span_base;
  const uintptr_t end = begin + HZ12_SPAN_BYTES;
  uint32_t count = 0u;
  void** link;
  H12Returned* returned;
  if (class_id >= HZ12_CLASS_COUNT || !span_base) return 0u;
  returned = &hz12_returned[class_id];
  hz12_mutex_lock(&returned->lock);
  link = &returned->head;
  while (*link) {
    void* current = *link;
    void* next = *(void**)current;
    const uintptr_t value = (uintptr_t)current;
    if (value >= begin && value < end) {
      *link = next;
      count += 1u;
    } else {
      link = (void**)current;
    }
  }
  hz12_mutex_unlock(&returned->lock);
  if (count != 0u) {
    HZ12_CLASS_DIAG_RETURNED_SINK_DEPTH(class_id, -(int32_t)count);
  }
  return count;
}

int hz12_span_route_detach(const void* span_base,
                           uint8_t expected_class_id) {
  const uintptr_t base = (uintptr_t)hz12_arena_base;
  const uintptr_t value = (uintptr_t)span_base;
  uintptr_t offset;
  size_t span_id;
  if (!hz12_arena_base || !span_base || value < base) return 0;
  offset = value - base;
  if (offset >= (uintptr_t)HZ12_ARENA_BYTES ||
      (offset & (HZ12_SPAN_BYTES - 1u)) != 0u) {
    return 0;
  }
  span_id = (size_t)(offset >> HZ12_SPAN_SHIFT);
  if (hz12_span_class[span_id] != (uint8_t)(expected_class_id + 1u)) {
    return 0;
  }
  hz12_span_class[span_id] = 0u;
  return 1;
}

int hz12_span_route_attach(const void* span_base, uint8_t class_id) {
  const uintptr_t base = (uintptr_t)hz12_arena_base;
  const uintptr_t value = (uintptr_t)span_base;
  uintptr_t offset;
  size_t span_id;
  if (class_id >= HZ12_CLASS_COUNT || !hz12_arena_base || !span_base ||
      value < base) {
    return 0;
  }
  offset = value - base;
  if (offset >= (uintptr_t)HZ12_ARENA_BYTES ||
      (offset & (HZ12_SPAN_BYTES - 1u)) != 0u) {
    return 0;
  }
  span_id = (size_t)(offset >> HZ12_SPAN_SHIFT);
  if (hz12_span_class[span_id] != 0u) return 0;
  hz12_span_class[span_id] = (uint8_t)(class_id + 1u);
  return 1;
}
