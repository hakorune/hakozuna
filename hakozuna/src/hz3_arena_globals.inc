// ----------------------------------------------------------------------------
// Global atomic pointers for lock-free range check (S12-4)
// Published in do_init: end first (release), then base (release)
// Read order in hot path: base first (acquire), then end (relaxed)
// ----------------------------------------------------------------------------
_Atomic(void*) g_hz3_arena_base = NULL;
_Atomic(void*) g_hz3_arena_end = NULL;

#if HZ3_ARENA_PRESSURE_BOX
// Pressure epoch: incremented on arena exhaustion, threads check and flush once per epoch.
_Atomic(uint32_t) g_hz3_arena_pressure_epoch = 0;
#endif

#if HZ3_OOM_SHOT || HZ3_OOM_FAILFAST
static _Atomic(int) g_hz3_arena_alloc_full_diag_fired = 0;
#endif

#if HZ3_S47_SEGMENT_QUARANTINE
// S47: Used slots counter (updated under g_hz3_arena_lock)
static _Atomic(uint32_t) g_hz3_arena_used_slots = 0;
// S47-PolicyBox: Generation counter for bounded wait (incremented on segment release)
static _Atomic(uint32_t) g_hz3_arena_free_gen = 0;
#if HZ3_S47_ARENA_GATE
// S47-2: ArenaGateBox - leader election gate (0=free, 1=leader active)
static _Atomic(uint32_t) g_hz3_arena_gate = 0;
// S47-2: Metrics (event-only, for triage)
static _Atomic(uint32_t) g_hz3_gate_leader_count = 0;
static _Atomic(uint32_t) g_hz3_gate_follower_wait_ok = 0;  // free_gen changed
static _Atomic(uint32_t) g_hz3_gate_follower_timeout = 0;  // waited but no change
static _Atomic(uint32_t) g_hz3_gate_seg_freed = 0;         // segments freed
#endif
#endif

#if HZ3_SMALL_V2_PTAG_ENABLE
// Page tag array: 0 = not ours, mmap'd in init
_Atomic(uint16_t)* g_hz3_page_tag = NULL;
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
// S17: 32-bit PageTagMap (dst/bin direct)
_Atomic(uint32_t)* g_hz3_page_tag32 = NULL;
#endif

#if HZ3_S265_PTAG16_LAZY_COMMIT
_Atomic(uint8_t)* g_hz3_s265_ptag16_commit_state = NULL;
uint32_t g_hz3_s265_ptag16_commit_pages = 0;

#if HZ3_S265_PTAG16_LAZY_STATS
static _Atomic(uint32_t) g_s265_ptag16_committed_pages = 0;
static _Atomic(uint64_t) g_s265_ptag16_ensure_calls = 0;
static _Atomic(uint64_t) g_s265_ptag16_commit_calls = 0;
static _Atomic(uint64_t) g_s265_ptag16_commit_fail = 0;
static _Atomic(uint64_t) g_s265_ptag16_wait_spins = 0;
static _Atomic(uint64_t) g_s265_ptag16_load_uncommitted = 0;
static _Atomic(int) g_s265_ptag16_atexit_registered = 0;
#define HZ3_S265_STAT_ADD(counter, value) \
    atomic_fetch_add_explicit(&(counter), (value), memory_order_relaxed)
#else
#define HZ3_S265_STAT_ADD(counter, value) do { } while (0)
#endif

static uint32_t hz3_s265_ptag16_meta_page(uint32_t page_idx) {
    return (uint32_t)(((size_t)page_idx * sizeof(_Atomic(uint16_t))) >>
                      HZ3_ARENA_PAGE_SHIFT);
}

#if HZ3_S265_PTAG16_LAZY_STATS
static void hz3_s265_ptag16_lazy_dump(void) {
    uint32_t committed_pages =
        atomic_load_explicit(&g_s265_ptag16_committed_pages, memory_order_relaxed);
    uint64_t ensure_calls =
        atomic_load_explicit(&g_s265_ptag16_ensure_calls, memory_order_relaxed);
    uint64_t commit_calls =
        atomic_load_explicit(&g_s265_ptag16_commit_calls, memory_order_relaxed);
    uint64_t commit_fail =
        atomic_load_explicit(&g_s265_ptag16_commit_fail, memory_order_relaxed);
    uint64_t wait_spins =
        atomic_load_explicit(&g_s265_ptag16_wait_spins, memory_order_relaxed);
    uint64_t load_uncommitted =
        atomic_load_explicit(&g_s265_ptag16_load_uncommitted, memory_order_relaxed);
    uint64_t committed_bytes = (uint64_t)committed_pages * HZ3_ARENA_PAGE_SIZE;
    uint64_t reserved_bytes =
        (uint64_t)HZ3_ARENA_MAX_PAGES * (uint64_t)sizeof(_Atomic(uint16_t));

    fprintf(stderr,
            "[HZ3_S265_PTAG16_LAZY] active=%d meta_pages=%u "
            "committed_pages=%u committed_bytes=%llu reserved_bytes=%llu "
            "ensure_calls=%llu commit_calls=%llu commit_fail=%llu "
            "wait_spins=%llu load_uncommitted=%llu\n",
            (int)(g_hz3_s265_ptag16_commit_state != NULL),
            (unsigned)g_hz3_s265_ptag16_commit_pages,
            (unsigned)committed_pages,
            (unsigned long long)committed_bytes,
            (unsigned long long)reserved_bytes,
            (unsigned long long)ensure_calls,
            (unsigned long long)commit_calls,
            (unsigned long long)commit_fail,
            (unsigned long long)wait_spins,
            (unsigned long long)load_uncommitted);
}

static void hz3_s265_ptag16_register_once(void) {
    if (atomic_exchange_explicit(&g_s265_ptag16_atexit_registered,
                                 1,
                                 memory_order_relaxed) == 0) {
        atexit(hz3_s265_ptag16_lazy_dump);
    }
}
#else
static inline void hz3_s265_ptag16_register_once(void) {}
#endif

#if HZ3_S266_PTAG16_LAZY_INLINE_FAST
#define HZ3_S265_ENSURE_NAME hz3_pagetag16_lazy_ensure_committed_slow
#define HZ3_S265_IS_COMMITTED_NAME hz3_pagetag16_lazy_is_committed_slow
#else
#define HZ3_S265_ENSURE_NAME hz3_pagetag16_lazy_ensure_committed
#define HZ3_S265_IS_COMMITTED_NAME hz3_pagetag16_lazy_is_committed
#endif

int HZ3_S265_ENSURE_NAME(uint32_t page_idx) {
    HZ3_S265_STAT_ADD(g_s265_ptag16_ensure_calls, 1);

    _Atomic(uint8_t)* state = g_hz3_s265_ptag16_commit_state;
    if (!state) {
        return 1;
    }

    uint32_t meta_page = hz3_s265_ptag16_meta_page(page_idx);
    if (meta_page >= g_hz3_s265_ptag16_commit_pages) {
        HZ3_S265_STAT_ADD(g_s265_ptag16_commit_fail, 1);
        return 0;
    }

    uint8_t cur = atomic_load_explicit(&state[meta_page], memory_order_acquire);
    if (cur == HZ3_S265_PTAG16_META_COMMITTED) {
        return 1;
    }

    uint8_t expected = HZ3_S265_PTAG16_META_UNCOMMITTED;
    if (atomic_compare_exchange_strong_explicit(&state[meta_page],
                                                &expected,
                                                HZ3_S265_PTAG16_META_COMMITTING,
                                                memory_order_acq_rel,
                                                memory_order_acquire)) {
        void* addr = (char*)g_hz3_page_tag +
                     ((size_t)meta_page << HZ3_ARENA_PAGE_SHIFT);
        void* committed = mmap(addr,
                               HZ3_ARENA_PAGE_SIZE,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                               -1,
                               0);
        if (committed == MAP_FAILED) {
            atomic_store_explicit(&state[meta_page],
                                  HZ3_S265_PTAG16_META_UNCOMMITTED,
                                  memory_order_release);
            HZ3_S265_STAT_ADD(g_s265_ptag16_commit_fail, 1);
            return 0;
        }

        HZ3_S265_STAT_ADD(g_s265_ptag16_commit_calls, 1);
        HZ3_S265_STAT_ADD(g_s265_ptag16_committed_pages, 1);
        atomic_store_explicit(&state[meta_page],
                              HZ3_S265_PTAG16_META_COMMITTED,
                              memory_order_release);
        return 1;
    }

    for (;;) {
        cur = atomic_load_explicit(&state[meta_page], memory_order_acquire);
        if (cur == HZ3_S265_PTAG16_META_COMMITTED) {
            return 1;
        }
        if (cur == HZ3_S265_PTAG16_META_UNCOMMITTED) {
            return HZ3_S265_ENSURE_NAME(page_idx);
        }
        HZ3_S265_STAT_ADD(g_s265_ptag16_wait_spins, 1);
        sched_yield();
    }
}

int HZ3_S265_IS_COMMITTED_NAME(uint32_t page_idx) {
    _Atomic(uint8_t)* state = g_hz3_s265_ptag16_commit_state;
    if (!state) {
        return 1;
    }

    uint32_t meta_page = hz3_s265_ptag16_meta_page(page_idx);
    if (meta_page >= g_hz3_s265_ptag16_commit_pages) {
        HZ3_S265_STAT_ADD(g_s265_ptag16_load_uncommitted, 1);
        return 0;
    }

    int committed =
        atomic_load_explicit(&state[meta_page], memory_order_acquire) ==
        HZ3_S265_PTAG16_META_COMMITTED;
    if (!committed) {
        HZ3_S265_STAT_ADD(g_s265_ptag16_load_uncommitted, 1);
    }
    return committed;
}

#undef HZ3_S265_ENSURE_NAME
#undef HZ3_S265_IS_COMMITTED_NAME
#endif

#if HZ3_S264_PTAG_STORE_OBS
static _Atomic(uint64_t) g_s264_ptag16_store = 0;
static _Atomic(uint64_t) g_s264_ptag16_clear = 0;
static _Atomic(uint64_t) g_s264_ptag32_store = 0;
static _Atomic(uint64_t) g_s264_ptag32_clear = 0;
static _Atomic(int) g_s264_ptag_atexit_registered = 0;

static void hz3_s264_ptag_store_dump(void) {
    uint64_t ptag16_store =
        atomic_load_explicit(&g_s264_ptag16_store, memory_order_relaxed);
    uint64_t ptag16_clear =
        atomic_load_explicit(&g_s264_ptag16_clear, memory_order_relaxed);
    uint64_t ptag32_store =
        atomic_load_explicit(&g_s264_ptag32_store, memory_order_relaxed);
    uint64_t ptag32_clear =
        atomic_load_explicit(&g_s264_ptag32_clear, memory_order_relaxed);

    fprintf(stderr,
            "[HZ3_S264_PTAG_STORE] "
            "ptag16_store=%llu ptag16_clear=%llu "
            "ptag32_store=%llu ptag32_clear=%llu "
            "ptag16_store_bytes=%llu ptag32_store_bytes=%llu\n",
            (unsigned long long)ptag16_store,
            (unsigned long long)ptag16_clear,
            (unsigned long long)ptag32_store,
            (unsigned long long)ptag32_clear,
            (unsigned long long)(ptag16_store * sizeof(uint16_t)),
            (unsigned long long)(ptag32_store * sizeof(uint32_t)));
}

static inline void hz3_s264_ptag_register_once(void) {
    if (atomic_exchange_explicit(&g_s264_ptag_atexit_registered,
                                 1,
                                 memory_order_relaxed) == 0) {
        atexit(hz3_s264_ptag_store_dump);
    }
}

void hz3_s264_ptag_note_store16(void) {
    hz3_s264_ptag_register_once();
    atomic_fetch_add_explicit(&g_s264_ptag16_store, 1, memory_order_relaxed);
}

void hz3_s264_ptag_note_clear16(void) {
    hz3_s264_ptag_register_once();
    atomic_fetch_add_explicit(&g_s264_ptag16_clear, 1, memory_order_relaxed);
}

void hz3_s264_ptag_note_store32(void) {
    hz3_s264_ptag_register_once();
    atomic_fetch_add_explicit(&g_s264_ptag32_store, 1, memory_order_relaxed);
}

void hz3_s264_ptag_note_clear32(void) {
    hz3_s264_ptag_register_once();
    atomic_fetch_add_explicit(&g_s264_ptag32_clear, 1, memory_order_relaxed);
}
#endif

typedef struct {
    size_t size;
    uint32_t slots;
    _Atomic uint8_t* used;
    uint32_t alloc_cursor;  // next slot search start (under g_hz3_arena_lock)
} Hz3ArenaState;

static Hz3ArenaState g_hz3_arena;
static hz3_once_t g_hz3_arena_once = HZ3_ONCE_INIT;
static hz3_lock_t g_hz3_arena_lock = HZ3_LOCK_INITIALIZER;
