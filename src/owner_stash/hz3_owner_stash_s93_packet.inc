// =============================================================================
// S93: Owner Stash PacketBox (research)
// =============================================================================
#if HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2

typedef struct Hz3S93StashPacket {
    struct Hz3S93StashPacket* next;
    uint16_t n;
    uint8_t sc;
    uint8_t _pad;
    void* ptrs[HZ3_S93_PACKET_K];
} Hz3S93StashPacket;

typedef struct {
    _Atomic(Hz3S93StashPacket*) free_head;
} Hz3S93PacketPool;

static Hz3S93PacketPool g_hz3_s93_pkt_pool[HZ3_NUM_SHARDS];

#if HZ3_S93_STATS
HZ3_DTOR_STATS_BEGIN(S93);
HZ3_DTOR_STAT(g_s93_pkt_alloc_calls);
HZ3_DTOR_STAT(g_s93_pkt_free_calls);
HZ3_DTOR_STAT(g_s93_pkt_alloc_slab_calls);
HZ3_DTOR_STAT(g_s93_pkt_alloc_fail_calls);
HZ3_DTOR_STAT(g_s93_pkt_inuse);
HZ3_DTOR_STAT(g_s93_pkt_highwater);
HZ3_DTOR_STAT(g_s93_push_calls);
HZ3_DTOR_STAT(g_s93_push_calls_n1);
HZ3_DTOR_STAT(g_s93_push_calls_skip_small_n);
HZ3_DTOR_STAT(g_s93_push_n_max);
HZ3_DTOR_STAT(g_s93_push_packets);
HZ3_DTOR_STAT(g_s93_push_objs);
HZ3_DTOR_STAT(g_s93_push_underfill_packets);
HZ3_DTOR_STAT(g_s93_push_underfill_min);
HZ3_DTOR_STAT(g_s93_pop_calls);
HZ3_DTOR_STAT(g_s93_pop_packets_consumed);
HZ3_DTOR_STAT(g_s93_pop_partial_packets);
HZ3_DTOR_ATEXIT_FLAG(g_s93);

static void hz3_s93_atexit_dump(void) {
    uint32_t alloc_calls = HZ3_DTOR_STAT_LOAD(g_s93_pkt_alloc_calls);
    uint32_t free_calls = HZ3_DTOR_STAT_LOAD(g_s93_pkt_free_calls);
    uint32_t slab_calls = HZ3_DTOR_STAT_LOAD(g_s93_pkt_alloc_slab_calls);
    uint32_t alloc_fail = HZ3_DTOR_STAT_LOAD(g_s93_pkt_alloc_fail_calls);
    uint32_t inuse = HZ3_DTOR_STAT_LOAD(g_s93_pkt_inuse);
    uint32_t highwater = HZ3_DTOR_STAT_LOAD(g_s93_pkt_highwater);
    uint32_t push_calls = HZ3_DTOR_STAT_LOAD(g_s93_push_calls);
    uint32_t push_calls_n1 = HZ3_DTOR_STAT_LOAD(g_s93_push_calls_n1);
    uint32_t push_calls_skip_small_n = HZ3_DTOR_STAT_LOAD(g_s93_push_calls_skip_small_n);
    uint32_t push_n_max = HZ3_DTOR_STAT_LOAD(g_s93_push_n_max);
    uint32_t push_packets = HZ3_DTOR_STAT_LOAD(g_s93_push_packets);
    uint32_t push_objs = HZ3_DTOR_STAT_LOAD(g_s93_push_objs);
    uint32_t underfill = HZ3_DTOR_STAT_LOAD(g_s93_push_underfill_packets);
    uint32_t underfill_min = HZ3_DTOR_STAT_LOAD(g_s93_push_underfill_min);
    uint32_t pop_calls = HZ3_DTOR_STAT_LOAD(g_s93_pop_calls);
    uint32_t pop_pkts = HZ3_DTOR_STAT_LOAD(g_s93_pop_packets_consumed);
    uint32_t pop_partial = HZ3_DTOR_STAT_LOAD(g_s93_pop_partial_packets);

    fprintf(stderr,
            "[HZ3_S93] K=%u min_n=%u slab=%u "
            "alloc=%u free=%u slab_calls=%u alloc_fail=%u "
            "inuse=%u highwater=%u "
            "push_calls=%u push_calls_n1=%u push_skip_small_n=%u push_n_max=%u "
            "push_pkts=%u push_objs=%u underfill_pkts=%u underfill_min=%u "
            "pop_calls=%u pop_pkts=%u pop_partial_pkts=%u\n",
            (unsigned)HZ3_S93_PACKET_K, (unsigned)HZ3_S93_PACKET_MIN_N, (unsigned)HZ3_S93_PACKET_SLAB_BYTES,
            alloc_calls, free_calls, slab_calls, alloc_fail,
            inuse, highwater,
            push_calls, push_calls_n1, push_calls_skip_small_n, push_n_max,
            push_packets, push_objs, underfill, underfill_min,
            pop_calls, pop_pkts, pop_partial);
}

static inline void hz3_s93_stat_inuse_inc(void) {
    uint32_t inuse = atomic_fetch_add_explicit(&g_s93_pkt_inuse, 1, memory_order_relaxed) + 1;
    uint32_t hw = atomic_load_explicit(&g_s93_pkt_highwater, memory_order_relaxed);
    while (inuse > hw &&
           !atomic_compare_exchange_weak_explicit(&g_s93_pkt_highwater, &hw, inuse,
                                                 memory_order_relaxed, memory_order_relaxed)) {
        // retry
    }
}

static inline void hz3_s93_stat_inuse_dec(void) {
    atomic_fetch_sub_explicit(&g_s93_pkt_inuse, 1, memory_order_relaxed);
}

static inline void hz3_s93_stat_push_n_max(uint32_t n) {
    uint32_t cur = atomic_load_explicit(&g_s93_push_n_max, memory_order_relaxed);
    while (n > cur &&
           !atomic_compare_exchange_weak_explicit(&g_s93_push_n_max, &cur, n,
                                                 memory_order_relaxed, memory_order_relaxed)) {
        // retry
    }
}

#define S93_STAT_INC(name) HZ3_DTOR_STAT_INC(name)
#define S93_STAT_ADD(name, val) HZ3_DTOR_STAT_ADD(name, (uint32_t)(val))
#define S93_ATEXIT_ONCE() HZ3_DTOR_ATEXIT_REGISTER_ONCE(g_s93, hz3_s93_atexit_dump)
#else
#define S93_STAT_INC(name) ((void)0)
#define S93_STAT_ADD(name, val) ((void)0)
#define S93_ATEXIT_ONCE() ((void)0)
static inline void hz3_s93_stat_inuse_inc(void) {}
static inline void hz3_s93_stat_inuse_dec(void) {}
static inline void hz3_s93_stat_push_n_max(uint32_t n) { (void)n; }
#endif

static int hz3_s93_pkt_pool_grow(uint8_t owner) {
    size_t slab_bytes = (size_t)HZ3_S93_PACKET_SLAB_BYTES;
    void* mem = mmap(NULL, slab_bytes, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        (void)errno;
        return 0;
    }

    size_t pkt_bytes = sizeof(Hz3S93StashPacket);
    uint32_t n_pkts = (uint32_t)(slab_bytes / pkt_bytes);
    if (n_pkts == 0) {
        // Should not happen if slab/pkt sizes are sane; leak mmap and fail-fast if requested.
#if HZ3_S93_FAILFAST
        fprintf(stderr, "[HZ3_S93_FAILFAST] where=pkt_pool_grow n_pkts=0 slab=%zu pkt=%zu\n",
                slab_bytes, pkt_bytes);
        abort();
#endif
        return 0;
    }

    Hz3S93StashPacket* pkts = (Hz3S93StashPacket*)mem;
    for (uint32_t i = 0; i + 1 < n_pkts; i++) {
        pkts[i].next = &pkts[i + 1];
    }
    pkts[n_pkts - 1].next = NULL;

    Hz3S93PacketPool* pool = &g_hz3_s93_pkt_pool[owner];
    Hz3S93StashPacket* old = atomic_load_explicit(&pool->free_head, memory_order_relaxed);
    do {
        pkts[n_pkts - 1].next = old;
    } while (!atomic_compare_exchange_weak_explicit(
        &pool->free_head, &old, pkts,
        memory_order_release, memory_order_relaxed));

    S93_STAT_INC(g_s93_pkt_alloc_slab_calls);
    return 1;
}

static Hz3S93StashPacket* hz3_s93_pkt_alloc(uint8_t owner) {
    Hz3S93PacketPool* pool = &g_hz3_s93_pkt_pool[owner];

    for (int attempt = 0; attempt < 2; attempt++) {
        Hz3S93StashPacket* head = atomic_load_explicit(&pool->free_head, memory_order_acquire);
        while (head) {
            Hz3S93StashPacket* next = head->next;
            if (atomic_compare_exchange_weak_explicit(
                    &pool->free_head, &head, next,
                    memory_order_acquire, memory_order_relaxed)) {
                head->next = NULL;
                head->n = 0;
                head->sc = 0;
                S93_STAT_INC(g_s93_pkt_alloc_calls);
                hz3_s93_stat_inuse_inc();
                return head;
            }
        }

        if (!hz3_s93_pkt_pool_grow(owner)) {
            break;
        }
    }

    S93_STAT_INC(g_s93_pkt_alloc_fail_calls);
    return NULL;
}

static void hz3_s93_pkt_free(uint8_t owner, Hz3S93StashPacket* pkt) {
    Hz3S93PacketPool* pool = &g_hz3_s93_pkt_pool[owner];

    hz3_s93_stat_inuse_dec();
    S93_STAT_INC(g_s93_pkt_free_calls);

    Hz3S93StashPacket* old = atomic_load_explicit(&pool->free_head, memory_order_relaxed);
    do {
        pkt->next = old;
    } while (!atomic_compare_exchange_weak_explicit(
        &pool->free_head, &old, pkt,
        memory_order_release, memory_order_relaxed));
}

#endif  // HZ3_S93_OWNER_STASH_PACKET || HZ3_S93_OWNER_STASH_PACKET_V2
