#include "hz11_span.h"

#include <stdatomic.h>
#include <pthread.h>
#include <sys/mman.h>

char* hz11_arena_base = NULL;
uint8_t hz11_span_class[HZ11_SPAN_COUNT]; /* BSS zero-fill == uncarved */
static _Atomic uint64_t hz11_span_create_count;

static char* hz11_arena_end = NULL;
static _Atomic(char*) hz11_span_cursor;
static pthread_once_t hz11_span_init_once = PTHREAD_ONCE_INIT;

/* Per-class returned-object sink. */
typedef struct H11Returned {
  pthread_mutex_t lock;
  void* head;
} H11Returned;
static H11Returned hz11_returned[HZ11_CLASS_COUNT];

static void hz11_span_init_real(void) {
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
  hz11_arena_base = (char*)base;
  hz11_arena_end = hz11_arena_base + HZ11_ARENA_BYTES;
  atomic_store_explicit(&hz11_span_cursor, hz11_arena_base, memory_order_relaxed);
  for (uint32_t i = 0; i < HZ11_CLASS_COUNT; ++i) {
    pthread_mutex_init(&hz11_returned[i].lock, NULL);
    hz11_returned[i].head = NULL;
  }
}

void hz11_span_init(void) {
  pthread_once(&hz11_span_init_once, hz11_span_init_real);
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

void hz11_returned_push(uint8_t class_id, void* ptr) {
  if (class_id >= HZ11_CLASS_COUNT || !ptr) {
    return;
  }
  H11Returned* r = &hz11_returned[class_id];
  pthread_mutex_lock(&r->lock);
  *(void**)ptr = r->head;
  r->head = ptr;
  pthread_mutex_unlock(&r->lock);
}

void* hz11_returned_pop(uint8_t class_id) {
  if (class_id >= HZ11_CLASS_COUNT) {
    return NULL;
  }
  H11Returned* r = &hz11_returned[class_id];
  pthread_mutex_lock(&r->lock);
  void* p = r->head;
  if (p) {
    r->head = *(void**)p;
  }
  pthread_mutex_unlock(&r->lock);
  return p;
}
