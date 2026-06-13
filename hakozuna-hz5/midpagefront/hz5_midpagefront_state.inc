// MidPageFront internal types, global maps, and TLS state.

typedef enum Hz5MidPageSlotState {
  HZ5_MIDPAGE_SLOT_LIVE = 0,
  HZ5_MIDPAGE_SLOT_CACHE = 1,
  HZ5_MIDPAGE_SLOT_REMOTE = 2,
  HZ5_MIDPAGE_SLOT_DEAD = 3,
} Hz5MidPageSlotState;

typedef struct Hz5MidPage {
  uint64_t magic;
  void* slab_base;
  uint32_t class_size;
  uint16_t class_index;
  uint16_t slot_count;
  Hz5OwnerToken owner;
  // active_bits is the canonical live-slot bitmap for the node-list path.
  // nodeless diagnostics use free_bits plus remote_bits instead.
  _Atomic uint64_t active_bits;
  _Atomic uint32_t live_count;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  // free_bits is owner-local reusable state. remote_bits claims cross-thread
  // frees until the owner drains them, so the two bitmaps must not overlap.
  _Atomic uint64_t free_bits;
  uint8_t in_partial;
  struct Hz5MidPage* next_partial;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REMOTE_SHADOW
  _Atomic uint64_t remote_bits;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  _Atomic uint64_t slot_state2;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET
  _Atomic uint64_t remote_packet_bits;
  _Atomic uint8_t remote_packet_queued;
  struct Hz5MidPage* remote_packet_next;
#endif
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  uint64_t pagerun_owner_free_bits;
  uint8_t pagerun_in_partial;
  struct Hz5MidPage* pagerun_next_partial;
#endif
  struct Hz5MidPage* next_owned;
  struct Hz5MidPage* source_next;
  struct Hz5MidPage* retired_next;
  uint8_t source_free;
  uint8_t empty_retained;
  uint8_t retired_empty;
} Hz5MidPage;

typedef struct Hz5MidPageNode {
  struct Hz5MidPageNode* next;
  Hz5MidPage* page;
} Hz5MidPageNode;

typedef struct Hz5MidPageMapEntry {
  _Atomic uintptr_t slab_base;
  _Atomic(Hz5MidPage*) page;
} Hz5MidPageMapEntry;

typedef struct Hz5MidPageRegion {
  uintptr_t base;
  Hz5MidPage* pages;
  uint32_t class_index;
  uint32_t slab_count;
} Hz5MidPageRegion;

typedef struct Hz5MidPageRegionMapEntry {
  _Atomic uintptr_t region_base;
  _Atomic(Hz5MidPageRegion*) region;
} Hz5MidPageRegionMapEntry;

typedef struct Hz5MidPageRawNode {
  struct Hz5MidPageRawNode* next;
} Hz5MidPageRawNode;

typedef struct Hz5MidPagePtrCacheEntry {
  void* ptr;
  Hz5MidPage* page;
  uint64_t mask;
} Hz5MidPagePtrCacheEntry;

typedef struct Hz5MidPageMagEntry {
  Hz5MidPage* page;
  uint16_t slot;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_UNSAFE_PTR_MAG || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M5_HIT_ONLY || \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  void* ptr;
#endif
} Hz5MidPageMagEntry;

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
typedef struct Hz5MidPageM6RawTls {
  void* raw_free[HZ5_MIDPAGEFRONT_M6_RAW_CAP];
  uint16_t raw_count;
} Hz5MidPageM6RawTls;
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING
typedef struct Hz5MidPageM10SlotEntry {
  Hz5MidPage* page;
  uint16_t slot;
} Hz5MidPageM10SlotEntry;

typedef struct Hz5MidPageM10SlotTls {
  Hz5MidPageM10SlotEntry slot_free[HZ5_MIDPAGEFRONT_M6_RAW_CAP];
  uint16_t slot_count;
} Hz5MidPageM10SlotTls;
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET
typedef struct Hz5MidPageM7RemoteBatch {
  Hz5MidPage* page;
  uint64_t bits;
} Hz5MidPageM7RemoteBatch;

typedef struct Hz5MidPageM7RemoteTls {
  Hz5MidPageM7RemoteBatch batch[HZ5_MIDPAGEFRONT_M7_REMOTE_PAGE_CAP];
  uint16_t batch_count;
  uint16_t ticket_count;
} Hz5MidPageM7RemoteTls;
#endif

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
typedef struct Hz5MidPageM8XferEntry {
  Hz5MidPage* page;
  uint64_t bits;
} Hz5MidPageM8XferEntry;

typedef struct Hz5MidPageM8XferTls {
  Hz5MidPageM8XferEntry xfer[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                             [HZ5_MIDPAGEFRONT_M8_XFER_PAGE_CAP];
  uint16_t xfer_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
} Hz5MidPageM8XferTls;
#endif

typedef struct Hz5MidPageTls {
  Hz5OwnerToken owner;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_HOT_SLOT
  void* hot_ptr[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* hot_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
  Hz5MidPageNode* free_head[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_RUN
  Hz5MidPage* current_page[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  uint64_t current_bits[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* partial_pages[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_NODELESS_PTRCACHE
  Hz5MidPagePtrCacheEntry ptrcache[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                                  [HZ5_MIDPAGEFRONT_NODELESS_PTRCACHE_CAP];
  uint32_t ptrcache_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE
  Hz5MidPageMagEntry magazine[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                              [HZ5_MIDPAGEFRONT_M4_MAG_CAP_MAX];
  uint16_t magazine_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN
  Hz5MidPage* pagerun_current[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* pagerun_partial[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE
  uint16_t empty_retained_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  uint16_t retired_empty_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
  Hz5MidPage* retired_empty[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_OVERFLOW_ARRAY
  Hz5MidPageMagEntry overflow[HZ5_MIDPAGEFRONT_CLASS_COUNT]
                             [HZ5_MIDPAGEFRONT_M4_OVERFLOW_CAP_MAX];
  uint16_t overflow_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_DRAIN_ON_HIT
  uint16_t remote_drain_hit_count[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
  void* m6_raw_free[HZ5_MIDPAGEFRONT_M6_RAW_CAP];
  uint16_t m6_raw_count;
#endif
#endif
  Hz5MidPage* owned_pages[HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_TLS_REGION_CACHE
  uintptr_t region_cache_base[HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP];
  Hz5MidPageRegion*
      region_cache_region[HZ5_MIDPAGEFRONT_TLS_REGION_CACHE_CAP];
  uint32_t region_cache_victim;
#endif
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  Hz5MidPageNode* remote_batch_head;
  Hz5MidPageNode* remote_batch_tail;
} Hz5MidPageTls;

static const uint32_t g_hz5_midpagefront_classes
    [HZ5_MIDPAGEFRONT_CLASS_COUNT] = {3072u,
                                      4096u,
                                      8192u,
                                      16384u,
                                      32768u
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_PAGERUN_64K
                                      ,
                                      65536u
#endif
};

static const Hz5OwnerToken k_hz5_midpagefront_no_owner = {0, 0};

#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static Hz5MidPageRegion
    g_hz5_midpagefront_regions[HZ5_MIDPAGEFRONT_REGION_CAP];
static Hz5MidPageRegionMapEntry
    g_hz5_midpagefront_region_map[HZ5_MIDPAGEFRONT_REGION_MAP_CAP];
static Hz5MidPage*
    g_hz5_midpagefront_source_free[HZ5_MIDPAGEFRONT_CLASS_COUNT];
static size_t g_hz5_midpagefront_region_count;
static pthread_mutex_t g_hz5_midpagefront_region_lock =
    PTHREAD_MUTEX_INITIALIZER;
#else
static Hz5MidPageMapEntry
    g_hz5_midpagefront_map[HZ5_MIDPAGEFRONT_MAP_CAP];
static pthread_mutex_t g_hz5_midpagefront_map_lock =
    PTHREAD_MUTEX_INITIALIZER;
#endif
static _Atomic(void*) g_hz5_midpagefront_owner_inbox[UINT16_MAX + 1u]
                                                  [HZ5_MIDPAGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_REMOTE_PACKET && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_CROSS_DRAIN
static _Atomic uint32_t g_hz5_midpagefront_owner_pending[UINT16_MAX + 1u];
#endif
static _Thread_local Hz5MidPageTls g_hz5_midpagefront_tls;
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_REMOTE_DEFERRED_FREE && \
    !BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M6_DEFERRED_FREE
static _Thread_local Hz5MidPageM6RawTls g_hz5_midpagefront_m6_remote_tls;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M10_REMOTE_SLOT_RING
static _Thread_local Hz5MidPageM10SlotTls g_hz5_midpagefront_m10_slot_tls;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M7_REMOTE_TICKET
static _Thread_local Hz5MidPageM7RemoteTls g_hz5_midpagefront_m7_remote_tls;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M8_OWNER_XFER
static _Thread_local Hz5MidPageM8XferTls g_hz5_midpagefront_m8_xfer_tls;
#endif
#if BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_EMPTY_SLAB_RELEASE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_M4_MAGAZINE && \
    BENCHLAB_HZ5_LINUX_MIDPAGEFRONT_REGION_ARRAY
static size_t hz5_midpagefront_m4_release_checkpoint(Hz5MidPageTls* tls);
static pthread_once_t g_hz5_midpagefront_tls_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t g_hz5_midpagefront_tls_key;
static _Atomic unsigned char g_hz5_midpagefront_tls_key_ready;

static void hz5_midpagefront_tls_destructor(void* value) {
  (void)value;
  (void)hz5_midpagefront_m4_release_checkpoint(&g_hz5_midpagefront_tls);
}

static void hz5_midpagefront_init_tls_key(void) {
  if (pthread_key_create(&g_hz5_midpagefront_tls_key,
                         hz5_midpagefront_tls_destructor) == 0) {
    atomic_store_explicit(&g_hz5_midpagefront_tls_key_ready,
                          1,
                          memory_order_release);
  }
}

static void hz5_midpagefront_register_tls_destructor(void) {
  pthread_once(&g_hz5_midpagefront_tls_key_once,
               hz5_midpagefront_init_tls_key);
  if (atomic_load_explicit(&g_hz5_midpagefront_tls_key_ready,
                           memory_order_acquire) == 0) {
    return;
  }
  if (!pthread_getspecific(g_hz5_midpagefront_tls_key)) {
    pthread_setspecific(g_hz5_midpagefront_tls_key, (void*)1);
  }
}
#else
static void hz5_midpagefront_register_tls_destructor(void) {}
#endif
