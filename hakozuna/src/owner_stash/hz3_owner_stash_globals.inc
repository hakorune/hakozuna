// =============================================================================
// HZ3_STASH_STRUCT_FAILFAST: Detect OOB/index corruption in stash structures
// =============================================================================
#ifndef HZ3_STASH_STRUCT_FAILFAST
#define HZ3_STASH_STRUCT_FAILFAST 0
#endif

#if HZ3_STASH_STRUCT_FAILFAST

static inline void hz3_stash_struct_fail(const char* where, const char* msg,
                                          uint8_t owner, int sc, int val) {
    fprintf(stderr, "[HZ3_STASH_STRUCT_FAILFAST] %s: %s (owner=%u sc=%d val=%d)\n",
            where, msg, owner, sc, val);
    abort();
}

// Check owner/sc ranges at function entry
#define STASH_CHECK_RANGE(where, owner, sc) do { \
    if ((owner) >= HZ3_NUM_SHARDS) { \
        hz3_stash_struct_fail(where, "owner OOB", owner, sc, (int)(owner)); \
    } \
    if ((sc) < 0 || (sc) >= HZ3_SMALL_NUM_SC) { \
        hz3_stash_struct_fail(where, "sc OOB", owner, sc, sc); \
    } \
} while(0)

// Check got doesn't exceed want (out[] bounds)
#define STASH_CHECK_OUT_BOUNDS(where, got, want, owner, sc) do { \
    if ((got) > (want)) { \
        hz3_stash_struct_fail(where, "got > want (out[] OOB)", owner, sc, (got)); \
    } \
} while(0)

// Check spill_count bounds (S67 spill arrays)
#ifdef HZ3_S67_SPILL_CAP
#define STASH_CHECK_SPILL_COUNT(where, cnt, owner, sc) do { \
    if ((cnt) > HZ3_S67_SPILL_CAP) { \
        hz3_stash_struct_fail(where, "spill_count > CAP", owner, sc, (int)(cnt)); \
    } \
} while(0)
#else
#define STASH_CHECK_SPILL_COUNT(where, cnt, owner, sc) ((void)0)
#endif

#else
#define STASH_CHECK_RANGE(where, owner, sc) ((void)0)
#define STASH_CHECK_OUT_BOUNDS(where, got, want, owner, sc) ((void)0)
#define STASH_CHECK_SPILL_COUNT(where, cnt, owner, sc) ((void)0)
#endif

// Global owner stash array
#if HZ3_OWNER_STASH_INSTANCES > 1
// S144: 3D array [owner][sc][inst] for N-way partitioning
static Hz3OwnerStashBin g_hz3_owner_stash[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC][HZ3_OWNER_STASH_INSTANCES];

// S144: Hash-based instance selection for push (load balancing)
// Mix seed/owner/sc to spread same thread across instances when owner/sc differ
static inline uint32_t hz3_s144_select_inst_push(uint32_t seed, uint8_t owner, int sc) {
    uint32_t h = seed ^ ((uint32_t)owner << 8) ^ (uint32_t)sc;
    h ^= h >> 12;  // light mix
    return h & HZ3_OWNER_STASH_INST_MASK;
}

// S144: Get bin for push (hash-based)
static inline Hz3OwnerStashBin* hz3_s144_get_bin_push(
    uint8_t owner, int sc, uint32_t seed)
{
    uint32_t inst = hz3_s144_select_inst_push(seed, owner, sc);
    return &g_hz3_owner_stash[owner][sc][inst];
}

// S144: Get bin for pop (round-robin)
static inline Hz3OwnerStashBin* hz3_s144_get_bin_pop(
    uint8_t owner, int sc, uint32_t* rr_cursor)
{
    uint32_t inst = *rr_cursor;
    *rr_cursor = (inst + 1) & HZ3_OWNER_STASH_INST_MASK;
    return &g_hz3_owner_stash[owner][sc][inst];
}

#else
// Baseline: 2D array [owner][sc]
static Hz3OwnerStashBin g_hz3_owner_stash[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];
#endif

static pthread_once_t g_hz3_owner_stash_once = PTHREAD_ONCE_INIT;

// =============================================================================
// S121: Page Worklist (MPSC queue of pages with pending remote objects)
// =============================================================================
#if HZ3_S121_PAGE_LOCAL_REMOTE

// =============================================================================
// S121-G: Tagged pointer helper macros (pack remote_head + state into one atomic)
// =============================================================================
#if HZ3_S121_G_ATOMIC_PACK
// Use lower 2 bits for state (pointers are 8-byte aligned on 64-bit)
#define HZ3_REM_MASK     ((uintptr_t)0x3)
#define HZ3_REM_PTR(x)   ((void*)((x) & ~HZ3_REM_MASK))
#define HZ3_REM_STATE(x) ((uint8_t)((x) & HZ3_REM_MASK))
#define HZ3_REM_MAKE(ptr, st) (((uintptr_t)(ptr)) | ((uintptr_t)(st) & HZ3_REM_MASK))

// States: 0=IDLE (notify on push), 1=ACTIVE (in pageq/processing)
#define HZ3_REM_STATE_IDLE   0
#define HZ3_REM_STATE_ACTIVE 1
#endif  // HZ3_S121_G_ATOMIC_PACK

// S121: Page header structure (duplicated from small_v2 for visibility)
// This must match Hz3SmallV2PageHdr in hz3_small_v2_types.inc
typedef struct Hz3S121PageHdr {
    uint32_t magic;
    uint8_t  owner;
    uint8_t  sc;
    uint16_t flags_or_live;
#if HZ3_S121_G_ATOMIC_PACK
    // S121-G: Packed remote_head + state into single atomic
    _Atomic(uintptr_t) remote_tagged;  // lower 2 bits = state, rest = ptr
    uint8_t            pad_g[8];       // padding to maintain struct size
#else
    _Atomic(void*)    remote_head;
    _Atomic(uint8_t)  remote_state;
    _Atomic(uint8_t)  remote_count;  // S121-A: lazy enqueue counter
    uint8_t           pad[6];
#endif
    void*             page_qnext;
} Hz3S121PageHdr;

typedef struct {
    _Atomic(void*) head;  // page header pointer (intrusive via page_qnext)
} Hz3PageWorklist;

static Hz3PageWorklist g_owner_pageq[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];

// pageq_push: MPSC push (multiple producers allowed)
static inline void hz3_pageq_push(uint8_t owner, uint8_t sc, void* page) {
    Hz3PageWorklist* q = &g_owner_pageq[owner][sc];
    Hz3S121PageHdr* hdr = (Hz3S121PageHdr*)page;
    _Atomic(void*)* headp = &q->head;

    void* old_head;
    do {
        old_head = atomic_load_explicit(headp, memory_order_relaxed);
        hdr->page_qnext = old_head;
    } while (!atomic_compare_exchange_weak_explicit(
        headp, &old_head, page,
        memory_order_release, memory_order_relaxed));

#if HZ3_S121_STATS
    atomic_fetch_add_explicit(&g_s121_pageq_enq, 1, memory_order_relaxed);
#endif
}

// pageq_pop: Single consumer (owner thread only)
// Uses CAS loop for correctness (MPSC: producers may push during pop)
static inline void* hz3_pageq_pop(uint8_t owner, uint8_t sc) {
    Hz3PageWorklist* q = &g_owner_pageq[owner][sc];

    void* page;
    do {
        page = atomic_load_explicit(&q->head, memory_order_acquire);
        if (!page) return NULL;
        Hz3S121PageHdr* hdr = (Hz3S121PageHdr*)page;
        void* next = hdr->page_qnext;
        if (atomic_compare_exchange_weak_explicit(
                &q->head, &page, next,
                memory_order_acq_rel, memory_order_relaxed)) {
            return page;
        }
    } while (1);
}

// S121-C: pageq_drain_all: Atomically take entire list (no CAS retry)
// Returns linked list of pages via page_qnext, or NULL if empty
// S121-K: Conditional drain - skip RMW if empty (ChatGPT Pro suggestion)
static inline void* hz3_pageq_drain_all(uint8_t owner, uint8_t sc) {
    Hz3PageWorklist* q = &g_owner_pageq[owner][sc];

    // Empty fastpath: relaxed load only (false-negative OK)
    void* h = atomic_load_explicit(&q->head, memory_order_relaxed);
    if (__builtin_expect(h == NULL, 1)) {
        return NULL;
    }

    // Non-empty: do the real detach
    return atomic_exchange_explicit(&q->head, NULL, memory_order_acq_rel);
}

// S121-C: pageq_push_list: Push a linked list of pages (batch requeue)
// list_head/list_tail are linked via page_qnext
static inline void hz3_pageq_push_list(uint8_t owner, uint8_t sc,
                                        void* list_head, void* list_tail) {
    if (!list_head) return;
    Hz3PageWorklist* q = &g_owner_pageq[owner][sc];
    Hz3S121PageHdr* tail_hdr = (Hz3S121PageHdr*)list_tail;
    _Atomic(void*)* headp = &q->head;

    void* old_head;
    do {
        old_head = atomic_load_explicit(headp, memory_order_relaxed);
        tail_hdr->page_qnext = old_head;
    } while (!atomic_compare_exchange_weak_explicit(
        headp, &old_head, list_head,
        memory_order_release, memory_order_relaxed));
}

#if HZ3_S121_M_PAGEQ_BATCH
// S121-M: Flush pending pages (link via page_qnext and push_list)
// Called when: batch full, owner/sc changes, or tcache cleanup
static inline void hz3_s121m_flush_pending(Hz3TCache* tc) {
    uint8_t count = tc->s121m_pending_count;
    if (count == 0) return;

    void** pages = tc->s121m_pending_pages;
    uint8_t owner = tc->s121m_pending_owner;
    uint8_t sc = tc->s121m_pending_sc;

    // Link pages via page_qnext
    for (uint8_t i = 0; i + 1 < count; i++) {
        Hz3S121PageHdr* hdr = (Hz3S121PageHdr*)pages[i];
        hdr->page_qnext = pages[i + 1];
    }
    // Last page's qnext will be set by push_list

    hz3_pageq_push_list(owner, sc, pages[0], pages[count - 1]);
    tc->s121m_pending_count = 0;
}

// S121-M: Add page to pending batch (flushes if full or owner/sc differs)
static inline void hz3_s121m_batch_add(Hz3TCache* tc, uint8_t owner, uint8_t sc, void* page) {
    // Flush if owner/sc differs
    if (tc->s121m_pending_count > 0 &&
        (tc->s121m_pending_owner != owner || tc->s121m_pending_sc != sc)) {
        hz3_s121m_flush_pending(tc);
    }

    // Store owner/sc if first page
    if (tc->s121m_pending_count == 0) {
        tc->s121m_pending_owner = owner;
        tc->s121m_pending_sc = sc;
    }

    // Add to batch
    tc->s121m_pending_pages[tc->s121m_pending_count++] = page;

    // Flush if full
    if (tc->s121m_pending_count >= HZ3_S121_M_BATCH_SIZE) {
        hz3_s121m_flush_pending(tc);
    }
}
#endif  // HZ3_S121_M_PAGEQ_BATCH

#endif  // HZ3_S121_PAGE_LOCAL_REMOTE



static void hz3_owner_stash_do_init(void) {
    for (int shard = 0; shard < HZ3_NUM_SHARDS; shard++) {
#if HZ3_S93_OWNER_STASH_PACKET
        atomic_init(&g_hz3_s93_pkt_pool[shard].free_head, NULL);
#endif
#if HZ3_S121_PAGE_LOCAL_REMOTE
        for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
            atomic_init(&g_owner_pageq[shard][sc].head, NULL);
        }
#endif
        for (int sc = 0; sc < HZ3_SMALL_NUM_SC; sc++) {
#if HZ3_OWNER_STASH_INSTANCES > 1
            // S144: Initialize all instances
            for (int inst = 0; inst < HZ3_OWNER_STASH_INSTANCES; inst++) {
                Hz3OwnerStashBin* bin = &g_hz3_owner_stash[shard][sc][inst];
                atomic_init(&bin->head, NULL);
                atomic_init(&bin->count, 0);
            }
#else
            Hz3OwnerStashBin* bin = &g_hz3_owner_stash[shard][sc];
            atomic_init(&bin->head, NULL);
            atomic_init(&bin->count, 0);
#endif
        }
    }
}

void hz3_owner_stash_init(void) {
    pthread_once(&g_hz3_owner_stash_once, hz3_owner_stash_do_init);
}

uint32_t hz3_owner_stash_count(uint8_t owner, int sc) {
    if (owner >= HZ3_NUM_SHARDS || sc < 0 || sc >= HZ3_SMALL_NUM_SC) {
        return 0;
    }
    hz3_owner_stash_init();
#if HZ3_OWNER_STASH_INSTANCES > 1
    // S144: Aggregate count across all instances
    uint32_t total = 0;
    for (int i = 0; i < HZ3_OWNER_STASH_INSTANCES; i++) {
        total += atomic_load_explicit(
            &g_hz3_owner_stash[owner][sc][i].count, memory_order_relaxed);
    }
    return total;
#else
    return atomic_load_explicit(&g_hz3_owner_stash[owner][sc].count, memory_order_relaxed);
#endif
}
