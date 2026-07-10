#ifndef HZ12_PERCPU_CACHE_H
#define HZ12_PERCPU_CACHE_H

/* HZ12PerCpuRseqCachePrototype-L2: opt-in per-CPU/rseq front cache.
 *
 * Inserts a CPU-local bounded slab between the thread-cache fast path and the
 * transfer/central path, on the fine128 base. When HZ12_PERCPU_RSEQ=0 (default)
 * the layer is behaviorally disabled through no-op stubs.
 *
 * This prototype uses rseq only to select the current CPU. Slab mutation is
 * protected by a per-CPU spinlock; there is no hand-rolled rseq critical section
 * in this box. If the rseq area cannot be verified at runtime, the layer
 * self-disables and behaves as identity fine128.
 * See docs/HZ12_PERCPU_RSEQ_CACHE_READINESS_L1.md and
 * docs/HZ12_PERCPU_RSEQ_CACHE_PROTOTYPE_L2.md for the boundary and gate. */

#include <stdint.h>
#include <stddef.h>
#include "hz12_size_class.h"

#ifndef HZ12_PERCPU_RSEQ
#define HZ12_PERCPU_RSEQ 0
#endif

#if HZ12_PERCPU_RSEQ

/* Per-(cpu,class) slab capacity (pointers). Kept small on purpose. */
#ifndef HZ12_PERCPU_CAP
#define HZ12_PERCPU_CAP 32u
#endif

/* Small-class prefix served by the per-CPU layer. Larger classes bypass to the
 * existing path. Default 12 covers up to ~2 KiB (sh6bench uses 1..1000 B). */
#ifndef HZ12_PERCPU_NCLASSES
#define HZ12_PERCPU_NCLASSES 12u
#endif
#if HZ12_PERCPU_NCLASSES > HZ12_CLASS_COUNT
#undef HZ12_PERCPU_NCLASSES
#define HZ12_PERCPU_NCLASSES HZ12_CLASS_COUNT
#endif

/* Static upper bound on CPU id (bounds the global array). */
#ifndef HZ12_PERCPU_MAX_CPUS
#define HZ12_PERCPU_MAX_CPUS 128u
#endif

/* Bounded rseq retry count before falling back to the existing path. */
#ifndef HZ12_PERCPU_RETRIES
#define HZ12_PERCPU_RETRIES 5u
#endif

typedef struct H12PerCpuClass {
  void* slots[HZ12_PERCPU_CAP];
  uint32_t count;
} H12PerCpuClass;

typedef struct H12PerCpu {
  H12PerCpuClass cls[HZ12_PERCPU_NCLASSES];
} H12PerCpu;

/* Process-wide enable flag. Set true only after the rseq area probe succeeds;
 * stays false (identity-fine128 behavior) on any uncertainty. Relaxed read on
 * the hot path; the fast-path hook gates the out-of-line pop/push call on it. */
extern _Atomic int hz12_percpu_enabled_flag;

static inline int hz12_percpu_enabled(void) {
  return __atomic_load_n(&hz12_percpu_enabled_flag, __ATOMIC_RELAXED);
}

/* Out-of-line implementations in hz12_percpu_cache.c. */
void hz12_percpu_init(void);   /* call once from the slow path / constructor */
int  hz12_percpu_pop(uint8_t class_id, void** out);  /* 1=hit, 0=miss/fallback */
int  hz12_percpu_push(uint8_t class_id, void* ptr);  /* 1=ok,  0=overflow/fallback */

#else /* !HZ12_PERCPU_RSEQ: compile to no-ops */

static inline int hz12_percpu_enabled(void) { return 0; }
static inline int hz12_percpu_pop(uint8_t class_id, void** out) {
  (void)class_id; (void)out; return 0;
}
static inline int hz12_percpu_push(uint8_t class_id, void* ptr) {
  (void)class_id; (void)ptr; return 0;
}
static inline void hz12_percpu_init(void) {}
static inline void hz12_percpu_dump(void) {}

#endif /* HZ12_PERCPU_RSEQ */

#endif /* HZ12_PERCPU_CACHE_H */
