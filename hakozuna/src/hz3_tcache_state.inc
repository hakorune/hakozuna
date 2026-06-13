// ============================================================================
// S40: Static assertions for SoA + pow2 padding
// ============================================================================
#if HZ3_BIN_PAD_LOG2
_Static_assert((HZ3_BIN_PAD & (HZ3_BIN_PAD - 1)) == 0, "HZ3_BIN_PAD must be power of 2");
_Static_assert(HZ3_BIN_TOTAL <= HZ3_BIN_PAD, "HZ3_BIN_TOTAL must fit in HZ3_BIN_PAD");
#endif

// ============================================================================
// TLS and Global Variables (shared via hz3_tcache_internal.h)
// ============================================================================

// Thread-local cache (zero-initialized by TLS semantics)
HZ3_TLS Hz3TCache t_hz3_cache;
#if HZ3_S200_INBOX_SEQ_GATE
HZ3_TLS uint32_t t_hz3_s200_inbox_seq_seen[HZ3_NUM_SC];
#endif

// Day 4: Shard assignment
_Atomic uint32_t g_shard_counter = 0;

// Detect shard collisions (multiple threads concurrently assigned same shard id).
static _Atomic int g_shard_collision_detected = 0;
static _Atomic uint32_t g_shard_live_count[HZ3_NUM_SHARDS] = {0};
static _Atomic uint8_t g_shard_exiting[HZ3_NUM_SHARDS] = {0};
#if HZ3_SHARD_COLLISION_SHOT
_Atomic int g_shard_collision_shot_fired = 0;
#endif

static inline void hz3_shard_exiting_store(uint8_t shard, uint8_t value) {
    if (shard >= HZ3_NUM_SHARDS) {
        return;
    }
    atomic_store_explicit(&g_shard_exiting[shard], value, memory_order_release);
}

// Day 5: Thread exit cleanup via pthread_key
pthread_key_t g_hz3_tcache_key;
pthread_once_t g_hz3_tcache_key_once = PTHREAD_ONCE_INIT;

#if HZ3_OOM_SHOT
// OOM triage: defined in hz3_tcache_stats.c, declared extern here
extern _Atomic uint64_t g_medium_page_alloc_sc[HZ3_NUM_SC];
#endif

#if HZ3_SEG_SCAVENGE_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
pthread_once_t g_scavenge_obs_atexit_once = PTHREAD_ONCE_INIT;
#endif

#if HZ3_S55_RETENTION_OBSERVE && !HZ3_SHIM_FORWARD_ONLY
pthread_once_t g_retention_obs_atexit_once = PTHREAD_ONCE_INIT;
#endif

#if HZ3_S62_ATEXIT_GATE
// S62-1A: AtExitGate - File-scope guards (follow existing atexit pattern)
pthread_once_t g_s62_atexit_once = PTHREAD_ONCE_INIT;
static _Atomic int g_s62_atexit_executed = 0;
#endif

#if HZ3_S56_ACTIVE_SET && HZ3_S56_ACTIVE_SET_STATS && !HZ3_SHIM_FORWARD_ONLY
_Atomic uint64_t g_s56_active_choose_alt = 0;
#endif

#if HZ3_LANE_SPLIT
static inline Hz3Lane* hz3_lane_alloc(void) {
    void* mem = mmap(NULL, HZ3_PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    return (Hz3Lane*)mem;
}

static inline void hz3_lane_free(Hz3Lane* lane) {
    if (!lane) {
        return;
    }
    munmap(lane, HZ3_PAGE_SIZE);
}
#endif
