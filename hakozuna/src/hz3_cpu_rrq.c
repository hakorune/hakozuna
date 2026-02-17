#define _GNU_SOURCE

#include "hz3_tcache_internal.h"

#include <sched.h>
#include <stdio.h>

#if HZ3_S220_CPU_RRQ
static _Atomic(void*) g_s220_rrq_head[HZ3_S220_CPU_RRQ_MAX_CPUS][HZ3_NUM_SC];
static _Atomic(uint32_t) g_s220_rrq_count[HZ3_S220_CPU_RRQ_MAX_CPUS][HZ3_NUM_SC];

#if HZ3_S220_CPU_RRQ_STATS
static _Thread_local uint64_t t_s220_cpu_index_calls = 0;
static _Thread_local uint64_t t_s220_cpu_sched_fail = 0;
static _Thread_local uint64_t t_s220_cpu_index_invalid = 0;
static _Thread_local uint64_t t_s220_cpu_index_ok = 0;
static _Thread_local uint64_t t_s220_push_attempts = 0;
static _Thread_local uint64_t t_s220_push_null = 0;
static _Thread_local uint64_t t_s220_push_sc_invalid = 0;
static _Thread_local uint64_t t_s220_push_sc_out_of_range = 0;
static _Thread_local uint64_t t_s220_push_cpu_fail = 0;
static _Thread_local uint64_t t_s220_push_cap_full = 0;
static _Thread_local uint64_t t_s220_push_count_cas_retry = 0;
static _Thread_local uint64_t t_s220_push_head_cas_retry = 0;
static _Thread_local uint64_t t_s220_push_ok = 0;
static _Thread_local uint64_t t_s220_pop_attempts = 0;
static _Thread_local uint64_t t_s220_pop_sc_invalid = 0;
static _Thread_local uint64_t t_s220_pop_sc_out_of_range = 0;
static _Thread_local uint64_t t_s220_pop_cpu_fail = 0;
static _Thread_local uint64_t t_s220_pop_empty = 0;
static _Thread_local uint64_t t_s220_pop_head_cas_retry = 0;
static _Thread_local uint64_t t_s220_pop_hit = 0;
static _Thread_local uint64_t t_s220_count_underflow = 0;
#if HZ3_S220_CPU_RRQ_STATS_SC
static _Thread_local uint64_t t_s220_push_ok_sc[HZ3_NUM_SC];
static _Thread_local uint64_t t_s220_pop_hit_sc[HZ3_NUM_SC];
#endif

static _Atomic uint64_t g_s220_cpu_index_calls = 0;
static _Atomic uint64_t g_s220_cpu_sched_fail = 0;
static _Atomic uint64_t g_s220_cpu_index_invalid = 0;
static _Atomic uint64_t g_s220_cpu_index_ok = 0;
static _Atomic uint64_t g_s220_push_attempts = 0;
static _Atomic uint64_t g_s220_push_null = 0;
static _Atomic uint64_t g_s220_push_sc_invalid = 0;
static _Atomic uint64_t g_s220_push_sc_out_of_range = 0;
static _Atomic uint64_t g_s220_push_cpu_fail = 0;
static _Atomic uint64_t g_s220_push_cap_full = 0;
static _Atomic uint64_t g_s220_push_count_cas_retry = 0;
static _Atomic uint64_t g_s220_push_head_cas_retry = 0;
static _Atomic uint64_t g_s220_push_ok = 0;
static _Atomic uint64_t g_s220_pop_attempts = 0;
static _Atomic uint64_t g_s220_pop_sc_invalid = 0;
static _Atomic uint64_t g_s220_pop_sc_out_of_range = 0;
static _Atomic uint64_t g_s220_pop_cpu_fail = 0;
static _Atomic uint64_t g_s220_pop_empty = 0;
static _Atomic uint64_t g_s220_pop_head_cas_retry = 0;
static _Atomic uint64_t g_s220_pop_hit = 0;
static _Atomic uint64_t g_s220_count_underflow = 0;
#if HZ3_S220_CPU_RRQ_STATS_SC
static _Atomic uint64_t g_s220_push_ok_sc[HZ3_NUM_SC];
static _Atomic uint64_t g_s220_pop_hit_sc[HZ3_NUM_SC];
#endif

static pthread_once_t g_s220_stats_once = PTHREAD_ONCE_INIT;

static void hz3_s220_rrq_stats_dump(void) {
    uint64_t push_attempts =
        atomic_load_explicit(&g_s220_push_attempts, memory_order_relaxed);
    uint64_t push_ok = atomic_load_explicit(&g_s220_push_ok, memory_order_relaxed);
    uint64_t pop_attempts =
        atomic_load_explicit(&g_s220_pop_attempts, memory_order_relaxed);
    uint64_t pop_hit = atomic_load_explicit(&g_s220_pop_hit, memory_order_relaxed);
    fprintf(stderr,
            "[HZ3_S220_RRQ] cpu_index_calls=%llu cpu_index_ok=%llu cpu_sched_fail=%llu cpu_index_invalid=%llu "
            "push_attempts=%llu push_ok=%llu push_cap_full=%llu push_null=%llu push_sc_invalid=%llu "
            "push_sc_out_of_range=%llu push_cpu_fail=%llu push_count_cas_retry=%llu push_head_cas_retry=%llu "
            "pop_attempts=%llu pop_hit=%llu pop_empty=%llu pop_sc_invalid=%llu pop_sc_out_of_range=%llu "
            "pop_cpu_fail=%llu pop_head_cas_retry=%llu count_underflow=%llu\n",
            (unsigned long long)atomic_load_explicit(&g_s220_cpu_index_calls, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_cpu_index_ok, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_cpu_sched_fail, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_cpu_index_invalid, memory_order_relaxed),
            (unsigned long long)push_attempts,
            (unsigned long long)push_ok,
            (unsigned long long)atomic_load_explicit(&g_s220_push_cap_full, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_push_null, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_push_sc_invalid, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_push_sc_out_of_range, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_push_cpu_fail, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_push_count_cas_retry, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_push_head_cas_retry, memory_order_relaxed),
            (unsigned long long)pop_attempts,
            (unsigned long long)pop_hit,
            (unsigned long long)atomic_load_explicit(&g_s220_pop_empty, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_pop_sc_invalid, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_pop_sc_out_of_range, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_pop_cpu_fail, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_pop_head_cas_retry, memory_order_relaxed),
            (unsigned long long)atomic_load_explicit(&g_s220_count_underflow, memory_order_relaxed));
#if HZ3_S220_CPU_RRQ_STATS_SC
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        uint64_t push_sc =
            atomic_load_explicit(&g_s220_push_ok_sc[sc], memory_order_relaxed);
        uint64_t pop_sc =
            atomic_load_explicit(&g_s220_pop_hit_sc[sc], memory_order_relaxed);
        if (push_sc == 0 && pop_sc == 0) {
            continue;
        }
        fprintf(stderr,
                "[HZ3_S220_RRQ_SC] sc=%u push_ok=%llu pop_hit=%llu\n",
                sc, (unsigned long long)push_sc, (unsigned long long)pop_sc);
    }
#endif
}

static void hz3_s220_rrq_stats_register(void) {
    atexit(hz3_s220_rrq_stats_dump);
}

void hz3_s220_rrq_register_once(void) {
    pthread_once(&g_s220_stats_once, hz3_s220_rrq_stats_register);
}

void hz3_s220_rrq_flush_tls(void) {
    atomic_fetch_add_explicit(&g_s220_cpu_index_calls, t_s220_cpu_index_calls,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_cpu_sched_fail, t_s220_cpu_sched_fail,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_cpu_index_invalid, t_s220_cpu_index_invalid,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_cpu_index_ok, t_s220_cpu_index_ok,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_attempts, t_s220_push_attempts,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_null, t_s220_push_null,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_sc_invalid, t_s220_push_sc_invalid,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_sc_out_of_range, t_s220_push_sc_out_of_range,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_cpu_fail, t_s220_push_cpu_fail,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_cap_full, t_s220_push_cap_full,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_count_cas_retry, t_s220_push_count_cas_retry,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_head_cas_retry, t_s220_push_head_cas_retry,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_push_ok, t_s220_push_ok,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_attempts, t_s220_pop_attempts,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_sc_invalid, t_s220_pop_sc_invalid,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_sc_out_of_range, t_s220_pop_sc_out_of_range,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_cpu_fail, t_s220_pop_cpu_fail,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_empty, t_s220_pop_empty,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_head_cas_retry, t_s220_pop_head_cas_retry,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_pop_hit, t_s220_pop_hit,
                              memory_order_relaxed);
    atomic_fetch_add_explicit(&g_s220_count_underflow, t_s220_count_underflow,
                              memory_order_relaxed);
#if HZ3_S220_CPU_RRQ_STATS_SC
    for (uint32_t sc = 0; sc < (uint32_t)HZ3_NUM_SC; sc++) {
        atomic_fetch_add_explicit(&g_s220_push_ok_sc[sc], t_s220_push_ok_sc[sc],
                                  memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s220_pop_hit_sc[sc], t_s220_pop_hit_sc[sc],
                                  memory_order_relaxed);
    }
#endif
}

#define S220_STAT_INC(name) (t_s220_##name++)
#if HZ3_S220_CPU_RRQ_STATS_SC
#define S220_STAT_INC_PUSH_SC(sc_) (t_s220_push_ok_sc[(sc_)]++)
#define S220_STAT_INC_POP_SC(sc_) (t_s220_pop_hit_sc[(sc_)]++)
#else
#define S220_STAT_INC_PUSH_SC(sc_) ((void)(sc_))
#define S220_STAT_INC_POP_SC(sc_) ((void)(sc_))
#endif
#else
void hz3_s220_rrq_register_once(void) {}
void hz3_s220_rrq_flush_tls(void) {}
#define S220_STAT_INC(name) ((void)0)
#define S220_STAT_INC_PUSH_SC(sc_) ((void)(sc_))
#define S220_STAT_INC_POP_SC(sc_) ((void)(sc_))
#endif

static inline int hz3_s220_rrq_cpu_index(void) {
    S220_STAT_INC(cpu_index_calls);
#if HZ3_S220_CPU_RRQ_CPU_ID_MODE == 1
    int cpu = (int)t_hz3_cache.my_shard;
#else
    int cpu = sched_getcpu();
    if (cpu < 0) {
        S220_STAT_INC(cpu_sched_fail);
        cpu = (int)t_hz3_cache.my_shard;
    }
#endif
    if (cpu < 0) {
        S220_STAT_INC(cpu_index_invalid);
        return -1;
    }
    S220_STAT_INC(cpu_index_ok);
    return (int)((uint32_t)cpu % (uint32_t)HZ3_S220_CPU_RRQ_MAX_CPUS);
}

static inline void hz3_s220_rrq_count_dec(_Atomic(uint32_t)* cell) {
    uint32_t cur = atomic_load_explicit(cell, memory_order_relaxed);
    while (cur > 0) {
        if (atomic_compare_exchange_weak_explicit(cell, &cur, cur - 1,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            return;
        }
    }
    S220_STAT_INC(count_underflow);
}
#endif

#if !HZ3_S220_CPU_RRQ
void hz3_s220_rrq_register_once(void) {}
void hz3_s220_rrq_flush_tls(void) {}
#endif

int hz3_s220_cpu_rrq_try_push(int sc, void* obj) {
#if HZ3_S220_CPU_RRQ
    S220_STAT_INC(push_attempts);
    if (!obj) {
        S220_STAT_INC(push_null);
        return 0;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        S220_STAT_INC(push_sc_invalid);
        return 0;
    }
    if (sc < HZ3_S220_CPU_RRQ_SC_MIN || sc > HZ3_S220_CPU_RRQ_SC_MAX) {
        S220_STAT_INC(push_sc_out_of_range);
        return 0;
    }

    int cpu = hz3_s220_rrq_cpu_index();
    if (cpu < 0) {
        S220_STAT_INC(push_cpu_fail);
        return 0;
    }

    _Atomic(uint32_t)* count_cell = &g_s220_rrq_count[cpu][sc];
    uint32_t cur = atomic_load_explicit(count_cell, memory_order_relaxed);
    for (;;) {
        if (cur >= (uint32_t)HZ3_S220_CPU_RRQ_CAP_PER_CPU_PER_SC) {
            S220_STAT_INC(push_cap_full);
            return 0;
        }
        if (atomic_compare_exchange_weak_explicit(count_cell, &cur, cur + 1,
                                                  memory_order_relaxed,
                                                  memory_order_relaxed)) {
            break;
        }
        S220_STAT_INC(push_count_cas_retry);
    }

    _Atomic(void*)* head_cell = &g_s220_rrq_head[cpu][sc];
    void* old_head = atomic_load_explicit(head_cell, memory_order_relaxed);
    for (;;) {
        hz3_obj_set_next(obj, old_head);
        if (atomic_compare_exchange_weak_explicit(head_cell, &old_head, obj,
                                                  memory_order_release,
                                                  memory_order_relaxed)) {
            S220_STAT_INC(push_ok);
            S220_STAT_INC_PUSH_SC(sc);
            return 1;
        }
        S220_STAT_INC(push_head_cas_retry);
    }
#else
    (void)sc;
    (void)obj;
    return 0;
#endif
}

int hz3_s220_cpu_rrq_pop_batch(int sc, void** out, int max_n) {
#if HZ3_S220_CPU_RRQ
    S220_STAT_INC(pop_attempts);
    if (!out || max_n <= 0) {
        return 0;
    }
    if ((uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        S220_STAT_INC(pop_sc_invalid);
        return 0;
    }
    if (sc < HZ3_S220_CPU_RRQ_SC_MIN || sc > HZ3_S220_CPU_RRQ_SC_MAX) {
        S220_STAT_INC(pop_sc_out_of_range);
        return 0;
    }

    int cpu = hz3_s220_rrq_cpu_index();
    if (cpu < 0) {
        S220_STAT_INC(pop_cpu_fail);
        return 0;
    }

    _Atomic(void*)* head_cell = &g_s220_rrq_head[cpu][sc];
    int popped = 0;
    while (popped < max_n) {
        void* head = atomic_load_explicit(head_cell, memory_order_acquire);
        if (!head) {
            if (popped == 0) {
                S220_STAT_INC(pop_empty);
            }
            break;
        }
        while (head != NULL) {
            void* next = hz3_obj_get_next(head);
            if (atomic_compare_exchange_weak_explicit(head_cell, &head, next,
                                                      memory_order_acquire,
                                                      memory_order_relaxed)) {
                hz3_obj_set_next(head, NULL);
                hz3_s220_rrq_count_dec(&g_s220_rrq_count[cpu][sc]);
                S220_STAT_INC(pop_hit);
                S220_STAT_INC_POP_SC(sc);
                out[popped++] = head;
                break;
            }
            S220_STAT_INC(pop_head_cas_retry);
        }
    }
    return popped;
#else
    (void)sc;
    (void)out;
    (void)max_n;
    return 0;
#endif
}

void* hz3_s220_cpu_rrq_try_pop(int sc) {
    void* out = NULL;
    return (hz3_s220_cpu_rrq_pop_batch(sc, &out, 1) == 1) ? out : NULL;
}
