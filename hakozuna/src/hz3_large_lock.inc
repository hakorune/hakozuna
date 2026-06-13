#if HZ3_S182_LARGE_CACHE_SPINLOCK
static atomic_flag g_hz3_large_cache_spin = ATOMIC_FLAG_INIT;

static inline void hz3_large_spin_pause_hint(void) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#else
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

static inline void hz3_large_spin_thread_yield(void) {
#if defined(_WIN32)
    SwitchToThread();
#else
    sched_yield();
#endif
}

static inline void hz3_large_cache_lock_acquire(void) {
    uint32_t spins = 0;
#if HZ3_S182_SPIN_ADAPTIVE_YIELD
    uint32_t next_yield_at = HZ3_S182_SPIN_YIELD_START;
    uint32_t stage = 0;
#endif
    while (atomic_flag_test_and_set_explicit(&g_hz3_large_cache_spin, memory_order_acquire)) {
        ++spins;
#if HZ3_S182_SPIN_PAUSE_INTERVAL > 0
        if ((spins % HZ3_S182_SPIN_PAUSE_INTERVAL) == 0u) {
            hz3_large_spin_pause_hint();
        }
#endif
#if HZ3_S182_SPIN_ADAPTIVE_YIELD
        if (spins >= next_yield_at) {
            hz3_large_spin_thread_yield();
            if (stage < HZ3_S182_SPIN_ADAPTIVE_MAX_STAGE) {
                ++stage;
            }
            next_yield_at += (HZ3_S182_SPIN_YIELD_INTERVAL << stage);
        }
#else
        if (spins >= HZ3_S182_SPIN_YIELD_START &&
            ((spins - HZ3_S182_SPIN_YIELD_START) % HZ3_S182_SPIN_YIELD_INTERVAL) == 0u) {
            hz3_large_spin_thread_yield();
        }
#endif
    }
}

static inline void hz3_large_cache_lock_release(void) {
    atomic_flag_clear_explicit(&g_hz3_large_cache_spin, memory_order_release);
}
#else
static hz3_lock_t g_hz3_large_lock = HZ3_LOCK_INITIALIZER;

static inline void hz3_large_cache_lock_acquire(void) {
    hz3_lock_acquire(&g_hz3_large_lock);
}

static inline void hz3_large_cache_lock_release(void) {
    hz3_lock_release(&g_hz3_large_lock);
}
#endif
