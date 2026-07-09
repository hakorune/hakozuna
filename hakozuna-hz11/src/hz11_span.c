#include "hz11_span.h"
#include "hz11_port.h"
#include "hz11_class_diag.h"

#include <stdatomic.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

char* hz11_arena_base = NULL;
uint8_t hz11_span_class[HZ11_SPAN_COUNT]; /* BSS zero-fill == uncarved */
static _Atomic uint64_t hz11_span_create_count;
#ifndef HZ11_SPAN_RETURNED_DIAG
#define HZ11_SPAN_RETURNED_DIAG 0
#endif
#if HZ11_SPAN_RETURNED_DIAG
static _Atomic uint64_t hz11_returned_push_count;
static _Atomic uint64_t hz11_returned_pop_hit_count;
static _Atomic uint64_t hz11_returned_pop_miss_count;
#endif

static char* hz11_arena_end = NULL;
static _Atomic(char*) hz11_span_cursor;
#if defined(_WIN32)
static INIT_ONCE hz11_span_init_once = INIT_ONCE_STATIC_INIT;
#else
static pthread_once_t hz11_span_init_once = PTHREAD_ONCE_INIT;
#endif

/* Per-class returned-object sink. */
typedef struct H11Returned {
  HZ11_MUTEX lock;
  void* head;
} H11Returned;
static H11Returned hz11_returned[HZ11_CLASS_COUNT];

static void hz11_span_init_real(void) {
#if defined(_WIN32)
  void* base = VirtualAlloc(NULL, (SIZE_T)HZ11_ARENA_BYTES,
                            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!base) {
    hz11_arena_base = NULL;
    hz11_arena_end = NULL;
    return;
  }
#else
  void* base = mmap(NULL, HZ11_ARENA_BYTES, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (base == MAP_FAILED) {
    hz11_arena_base = NULL;
    hz11_arena_end = NULL;
    return;
  }
#if HZ11_ARENA_NOHUGEPAGE
  (void)madvise(base, HZ11_ARENA_BYTES, MADV_NOHUGEPAGE);
#endif
#endif
  hz11_arena_base = (char*)base;
  hz11_arena_end = hz11_arena_base + HZ11_ARENA_BYTES;
  atomic_store_explicit(&hz11_span_cursor, hz11_arena_base, memory_order_relaxed);
  for (uint32_t i = 0; i < HZ11_CLASS_COUNT; ++i) {
    hz11_mutex_init(&hz11_returned[i].lock);
    hz11_returned[i].head = NULL;
  }
}

#if defined(_WIN32)
static BOOL CALLBACK hz11_span_init_once_callback(PINIT_ONCE init_once,
                                                  PVOID parameter,
                                                  PVOID* context) {
  (void)init_once;
  (void)parameter;
  (void)context;
  hz11_span_init_real();
  return TRUE;
}
#endif

void hz11_span_init(void) {
#if defined(_WIN32)
  (void)InitOnceExecuteOnce(&hz11_span_init_once,
                            hz11_span_init_once_callback,
                            NULL,
                            NULL);
#else
  pthread_once(&hz11_span_init_once, hz11_span_init_real);
#endif
}

void* hz11_span_carve_for_class(uint8_t class_id) {
  hz11_span_init();
  if (!hz11_arena_base) {
    return NULL;
  }
  for (;;) {
    char* cur = atomic_load_explicit(&hz11_span_cursor, memory_order_relaxed);
    if (cur + HZ11_SPAN_BYTES > hz11_arena_end) {
      return NULL; /* arena full */
    }
    char* nxt = cur + HZ11_SPAN_BYTES;
    if (atomic_compare_exchange_weak_explicit(&hz11_span_cursor, &cur, nxt,
                                              memory_order_acq_rel,
                                              memory_order_relaxed)) {
      size_t id = ((size_t)(cur - hz11_arena_base)) >> HZ11_SPAN_SHIFT;
      hz11_span_class[id] = (uint8_t)(class_id + 1u);
      atomic_fetch_add_explicit(&hz11_span_create_count, 1u,
                                memory_order_relaxed);
      return cur;
    }
  }
}

uint64_t hz11_span_create_count_load(void) {
  return atomic_load_explicit(&hz11_span_create_count, memory_order_relaxed);
}

uint64_t hz11_returned_push_count_load(void) {
#if HZ11_SPAN_RETURNED_DIAG
  return atomic_load_explicit(&hz11_returned_push_count, memory_order_relaxed);
#else
  return 0u;
#endif
}

uint64_t hz11_returned_pop_hit_count_load(void) {
#if HZ11_SPAN_RETURNED_DIAG
  return atomic_load_explicit(&hz11_returned_pop_hit_count, memory_order_relaxed);
#else
  return 0u;
#endif
}

uint64_t hz11_returned_pop_miss_count_load(void) {
#if HZ11_SPAN_RETURNED_DIAG
  return atomic_load_explicit(&hz11_returned_pop_miss_count, memory_order_relaxed);
#else
  return 0u;
#endif
}

void hz11_returned_push(uint8_t class_id, void* ptr) {
  if (class_id >= HZ11_CLASS_COUNT || !ptr) {
    return;
  }
  H11Returned* r = &hz11_returned[class_id];
  hz11_mutex_lock(&r->lock);
  *(void**)ptr = r->head;
  r->head = ptr;
  hz11_mutex_unlock(&r->lock);
#if HZ11_SPAN_RETURNED_DIAG
  atomic_fetch_add_explicit(&hz11_returned_push_count, 1u,
                            memory_order_relaxed);
#endif
}

void* hz11_returned_pop(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
#if HZ11_SPAN_RETURNED_DIAG
    atomic_fetch_add_explicit(&hz11_returned_pop_miss_count, 1u,
                              memory_order_relaxed);
#endif
    return NULL;
  }
  H11Returned* r = &hz11_returned[class_id];
  hz11_mutex_lock(&r->lock);
  void* p = r->head;
  if (p) {
    r->head = *(void**)p;
  }
  hz11_mutex_unlock(&r->lock);
#if HZ11_SPAN_RETURNED_DIAG
  atomic_fetch_add_explicit(p ? &hz11_returned_pop_hit_count
                              : &hz11_returned_pop_miss_count,
                            1u,
                            memory_order_relaxed);
#endif
  if (p) {
    HZ11_CLASS_DIAG_RETURNED_POP_HIT(class_id);
  } else {
    HZ11_CLASS_DIAG_RETURNED_POP_MISS(class_id);
  }
  return p;
}

uint32_t hz11_returned_pop_range(uint8_t class_id, void** out, uint32_t max) {
  if (class_id >= HZ11_CLASS_COUNT || !out || max == 0u) {
    return 0u;
  }
  H11Returned* r = &hz11_returned[class_id];
  hz11_mutex_lock(&r->lock);
  uint32_t n = 0u;
  while (n < max && r->head) {
    void* p = r->head;
    r->head = *(void**)p;
    out[n++] = p;
  }
  hz11_mutex_unlock(&r->lock);
#if HZ11_SPAN_RETURNED_DIAG
  if (n > 0u) {
    atomic_fetch_add_explicit(&hz11_returned_pop_hit_count, n,
                              memory_order_relaxed);
  } else {
    atomic_fetch_add_explicit(&hz11_returned_pop_miss_count, 1u,
                              memory_order_relaxed);
  }
#endif
#if HZ11_CLASS_DIAG
  if (n > 0u) {
    for (uint32_t i = 0u; i < n; ++i) {
      HZ11_CLASS_DIAG_RETURNED_POP_HIT(class_id);
    }
  } else {
    HZ11_CLASS_DIAG_RETURNED_POP_MISS(class_id);
  }
#endif
  return n;
}
