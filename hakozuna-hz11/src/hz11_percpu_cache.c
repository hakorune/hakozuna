/* HZ11PerCpuRseqCachePrototype-L2: correctness-first locked per-CPU cache.
 *
 * IMPORTANT: this prototype uses rseq ONLY to select the current CPU (glibc's
 * per-thread rseq area `cpu_id`). The per-CPU slab mutation is serialized by a
 * small per-CPU spinlock for correctness. There is NO hand-rolled rseq critical
 * section in this box. The lock-free single-commit rseq CS is explicitly deferred
 * to HZ11PerCpuRseqSingleCommitCS-L2, gated on this box's measurement.
 *
 * See docs/HZ11_PERCPU_RSEQ_CACHE_READINESS_L1.md and
 * docs/HZ11_PERCPU_RSEQ_CACHE_PROTOTYPE_L2.md for the boundary and gate. */

#include "hz11_percpu_cache.h"

#if HZ11_PERCPU_RSEQ

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && \
    defined(__GNUC__)
#include <sys/rseq.h>
#define HZ11_PERCPU_HAVE_RSEQ 1
#else
#define HZ11_PERCPU_HAVE_RSEQ 0
#endif

#ifndef RSEQ_CPU_ID_UNINITIALIZED
#define RSEQ_CPU_ID_UNINITIALIZED (-1)
#endif
#ifndef RSEQ_CPU_ID_REGISTRATION_FAILED
#define RSEQ_CPU_ID_REGISTRATION_FAILED (-2)
#endif

/* Process-wide enable flag. 0 = identity-fine128 (percpu ops return miss).
 * Set to 1 once by the lazy probe, only if the rseq area is verified. */
_Atomic int hz11_percpu_enabled_flag = 0;

static H11PerCpu g_percpu[HZ11_PERCPU_MAX_CPUS];
static _Atomic int g_lock[HZ11_PERCPU_MAX_CPUS]; /* one spinlock per CPU */

/* Counters (relaxed; dumped via HZ11_PERCPU_STATS=1 atexit). */
static _Atomic uint64_t c_rseq_enabled;
static _Atomic uint64_t c_rseq_fallback;
static _Atomic uint64_t c_percpu_hit;
static _Atomic uint64_t c_percpu_miss;
static _Atomic uint64_t c_percpu_flush;
static _Atomic uint64_t c_percpu_abort; /* reserved; locked lane does not abort */

static inline void cnt_inc(_Atomic uint64_t* s) {
  atomic_fetch_add_explicit(s, 1u, memory_order_relaxed);
}

static inline void hz11_percpu_spin_lock(_Atomic int* l) {
  for (;;) {
    int expected = 0;
    if (atomic_compare_exchange_weak_explicit(l, &expected, 1,
                                              memory_order_acquire,
                                              memory_order_relaxed)) {
      return;
    }
    while (atomic_load_explicit(l, memory_order_relaxed)) {
#if defined(__x86_64__)
      __asm__ __volatile__("pause" ::: "memory");
#else
      __asm__ __volatile__("" ::: "memory");
#endif
    }
  }
}

static inline void hz11_percpu_spin_unlock(_Atomic int* l) {
  atomic_store_explicit(l, 0, memory_order_release);
}

#if HZ11_PERCPU_HAVE_RSEQ
static inline struct rseq* hz11_percpu_area(void) {
  /* glibc registers the per-thread rseq area; __rseq_offset is from the thread
   * pointer. Reuse glibc's registration (do not call syscall(__NR_rseq)). */
  return (struct rseq*)((char*)__builtin_thread_pointer() + (size_t)__rseq_offset);
}

static inline int hz11_percpu_cpu_id(void) {
  int c = (int)hz11_percpu_area()->cpu_id;
  if (c < 0) {
    return -1; /* UNINITIALIZED / REGISTRATION_FAILED */
  }
  return c;
}
#endif /* HZ11_PERCPU_HAVE_RSEQ */

static _Atomic int hz11_percpu_probed = 0;

static void hz11_percpu_dump(void);

/* Lazy probe: called once (winner of the CAS) on first percpu use. Disables on
 * ANY uncertainty. */
void hz11_percpu_init(void) {
#if HZ11_PERCPU_HAVE_RSEQ
  if (__rseq_size == 0u) {
    cnt_inc(&c_rseq_fallback);
    return;
  }
  int c = (int)hz11_percpu_area()->cpu_id;
  if (c == RSEQ_CPU_ID_UNINITIALIZED || c == RSEQ_CPU_ID_REGISTRATION_FAILED) {
    cnt_inc(&c_rseq_fallback);
    return;
  }
  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&hz11_percpu_enabled_flag, &expected,
                                              1, memory_order_release,
                                              memory_order_relaxed)) {
    cnt_inc(&c_rseq_enabled);
    (void)atexit(hz11_percpu_dump);
  }
#else
  cnt_inc(&c_rseq_fallback);
#endif
}

static inline void hz11_percpu_ensure(void) {
  if (atomic_load_explicit(&hz11_percpu_enabled_flag, memory_order_relaxed)) {
    return;
  }
  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&hz11_percpu_probed, &expected, 1,
                                              memory_order_acq_rel,
                                              memory_order_relaxed)) {
    hz11_percpu_init();
  }
}

int hz11_percpu_pop(uint8_t cid, void** out) {
  if (out) {
    *out = NULL;
  }
  hz11_percpu_ensure();
  if (!hz11_percpu_enabled()) {
    return 0;
  }
  if (cid >= HZ11_PERCPU_NCLASSES) {
    cnt_inc(&c_percpu_miss);
    return 0;
  }
#if HZ11_PERCPU_HAVE_RSEQ
  int cpu = hz11_percpu_cpu_id();
  if (cpu < 0 || (uint32_t)cpu >= HZ11_PERCPU_MAX_CPUS) {
    cnt_inc(&c_rseq_fallback);
    return 0;
  }
  H11PerCpuClass* slab = &g_percpu[cpu].cls[cid];
  void* p = NULL;
  int hit = 0;
  hz11_percpu_spin_lock(&g_lock[cpu]);
  if (slab->count > 0u) {
    slab->count -= 1u;
    p = slab->slots[slab->count];
    hit = 1;
  }
  hz11_percpu_spin_unlock(&g_lock[cpu]);
  if (hit) {
    if (out) {
      *out = p;
    }
    cnt_inc(&c_percpu_hit);
    return 1;
  }
  cnt_inc(&c_percpu_miss);
#else
  (void)cid;
#endif
  return 0;
}

int hz11_percpu_push(uint8_t cid, void* ptr) {
  hz11_percpu_ensure();
  if (!hz11_percpu_enabled()) {
    return 0;
  }
  if (cid >= HZ11_PERCPU_NCLASSES) {
    return 0; /* ineligible: caller's thread-cache push handles it */
  }
#if HZ11_PERCPU_HAVE_RSEQ
  int cpu = hz11_percpu_cpu_id();
  if (cpu < 0 || (uint32_t)cpu >= HZ11_PERCPU_MAX_CPUS) {
    cnt_inc(&c_rseq_fallback);
    return 0;
  }
  H11PerCpuClass* slab = &g_percpu[cpu].cls[cid];
  int ok = 0;
  hz11_percpu_spin_lock(&g_lock[cpu]);
  if (slab->count < HZ11_PERCPU_CAP) {
    slab->slots[slab->count] = ptr;
    slab->count += 1u;
    ok = 1;
  }
  hz11_percpu_spin_unlock(&g_lock[cpu]);
  if (ok) {
    cnt_inc(&c_percpu_hit);
    return 1;
  }
  cnt_inc(&c_percpu_flush); /* overflow: caller flushes to the thread cache */
#else
  (void)cid;
  (void)ptr;
#endif
  return 0;
}

static void hz11_percpu_dump(void) {
  /* Env-gated so normal benchmark output is unpolluted. */
  if (!getenv("HZ11_PERCPU_STATS")) {
    return;
  }
  fprintf(stderr,
          "[hz11-percpu] enabled=%d rseq_enabled=%llu rseq_fallback=%llu "
          "hit=%llu miss=%llu flush=%llu abort=%llu\n",
          hz11_percpu_enabled() ? 1 : 0,
          (unsigned long long)atomic_load_explicit(&c_rseq_enabled,
                                                   memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&c_rseq_fallback,
                                                   memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&c_percpu_hit,
                                                   memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&c_percpu_miss,
                                                   memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&c_percpu_flush,
                                                   memory_order_relaxed),
          (unsigned long long)atomic_load_explicit(&c_percpu_abort,
                                                   memory_order_relaxed));
}

#else  /* !HZ11_PERCPU_RSEQ */
typedef int hz11_percpu_empty_translation_unit; /* keep TU non-empty when disabled */
#endif /* HZ11_PERCPU_RSEQ */
