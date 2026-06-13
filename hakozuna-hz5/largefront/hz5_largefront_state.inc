typedef enum Hz5LargeSpanState {
  HZ5_LARGESPAN_INVALID = 0,
  HZ5_LARGESPAN_ACTIVE = 1,
  HZ5_LARGESPAN_LOCAL_FREE = 2,
  HZ5_LARGESPAN_REMOTE_PENDING = 3,
  HZ5_LARGESPAN_ORPHAN = 4,
  HZ5_LARGESPAN_TRANSFER_FREE = 5
} Hz5LargeSpanState;

typedef struct Hz5LargeSpan {
  uint64_t magic;
  void* raw;
  void* base;
  uint32_t class_bytes;
  uint16_t page_count;
  uint16_t class_index;
  Hz5OwnerToken owner;
  _Atomic unsigned char state;
  uint16_t generation;
  uint16_t flags;
  struct Hz5LargeSpan* next;
} Hz5LargeSpan;

typedef struct Hz5LargeMapEntry {
  _Atomic uintptr_t page_base;
  _Atomic(Hz5LargeSpan*) span;
} Hz5LargeMapEntry;

typedef struct Hz5LargeRegion {
  uintptr_t base;
  uintptr_t end;
  uint32_t class_index;
  uint32_t span_count;
  size_t stride;
} Hz5LargeRegion;

typedef struct Hz5LargeRegionLink {
  uintptr_t key;
  const Hz5LargeRegion* region;
  struct Hz5LargeRegionLink* next;
} Hz5LargeRegionLink;

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
typedef struct Hz5LargeRemoteChunk {
  struct Hz5LargeRemoteChunk* next;
  uint16_t count;
  uint16_t class_index;
  Hz5OwnerToken owner;
  Hz5LargeSpan* spans[HZ5_LARGEFRONT_REMOTE_CHUNK_CAP];
} Hz5LargeRemoteChunk;
#endif

typedef struct Hz5LargeTls {
  Hz5OwnerToken owner;
  Hz5LargeSpan* free_head[HZ5_LARGEFRONT_CLASS_COUNT];
  uint32_t free_count[HZ5_LARGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_HOLD
  Hz5LargeSpan* remote_hold_head[HZ5_LARGEFRONT_CLASS_COUNT];
  Hz5LargeSpan* remote_hold_tail[HZ5_LARGEFRONT_CLASS_COUNT];
  uint32_t remote_hold_count[HZ5_LARGEFRONT_CLASS_COUNT];
#endif
  Hz5OwnerToken remote_batch_owner;
  uint32_t remote_batch_class;
  uint32_t remote_batch_count;
  uint32_t remote_batch_cap;
  Hz5LargeSpan* remote_batch_head;
  Hz5LargeSpan* remote_batch_tail;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
  uint32_t transfer128_count;
  Hz5LargeSpan* transfer128_head;
  Hz5LargeSpan* transfer128_tail;
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
  Hz5LargeRemoteChunk* chunk_pool;
  uint32_t chunk_next;
  uint32_t chunk_cap;
#endif
} Hz5LargeTls;

typedef struct Hz5LargeRawNode {
  struct Hz5LargeRawNode* next;
} Hz5LargeRawNode;

static const uint32_t g_hz5_largefront_classes[HZ5_LARGEFRONT_CLASS_COUNT] = {
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_LOWER_CLASSES
    8192u, 16384u, 32768u, 65536u,
#endif
    131072u, 262144u, 524288u, 1048576u};

static const Hz5OwnerToken k_hz5_largefront_no_owner = {0, 0};

static Hz5LargeMapEntry g_hz5_largefront_map[HZ5_LARGEFRONT_MAP_CAP];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_BASE_DIRECTMAP
static Hz5LargeMapEntry
    g_hz5_largefront_base_directmap[HZ5_LARGEFRONT_MAP_CAP];
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REGION_MAP
static Hz5LargeRegion g_hz5_largefront_regions[HZ5_LARGEFRONT_REGION_CAP];
static Hz5LargeRegionLink
    g_hz5_largefront_region_links[HZ5_LARGEFRONT_REGION_LINK_CAP];
static _Atomic(Hz5LargeRegionLink*)
    g_hz5_largefront_region_buckets[HZ5_LARGEFRONT_REGION_BUCKET_CAP];
static size_t g_hz5_largefront_region_count;
static size_t g_hz5_largefront_region_link_count;
static pthread_mutex_t g_hz5_largefront_region_lock =
    PTHREAD_MUTEX_INITIALIZER;
#endif
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_OWNER_INBOX
static _Atomic(void*) g_hz5_largefront_owner_inbox[UINT16_MAX + 1u]
                                                 [HZ5_LARGEFRONT_CLASS_COUNT];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_REMOTE_CHUNK_INBOX
static _Atomic(Hz5LargeRemoteChunk*)
    g_hz5_largefront_owner_chunk_inbox[UINT16_MAX + 1u]
                                      [HZ5_LARGEFRONT_CLASS_COUNT];
#endif
#endif
static Hz5LargeSpan* g_hz5_largefront_global_free[HZ5_LARGEFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_largefront_global_lock =
    PTHREAD_MUTEX_INITIALIZER;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128
static Hz5LargeSpan* g_hz5_largefront_transfer128_head;
static uint32_t g_hz5_largefront_transfer128_count;
static pthread_mutex_t g_hz5_largefront_transfer128_lock =
    PTHREAD_MUTEX_INITIALIZER;
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_OWNER_SHARD || \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_CONSUMER_SHARD
static Hz5LargeSpan*
    g_hz5_largefront_transfer128_shard_head[HZ5_LARGEFRONT_TRANSFER128_SHARD_COUNT];
static uint32_t
    g_hz5_largefront_transfer128_shard_count[HZ5_LARGEFRONT_TRANSFER128_SHARD_COUNT];
static _Atomic unsigned char
    g_hz5_largefront_transfer128_shard_lock[HZ5_LARGEFRONT_TRANSFER128_SHARD_COUNT];
#if BENCHLAB_HZ5_LINUX_LARGEFRONT_TRANSFER128_CONSUMER_SHARD
static _Atomic uint32_t g_hz5_largefront_transfer128_push_shard_cursor;
static _Atomic uint64_t g_hz5_largefront_transfer128_nonempty_mask;
#endif
#endif
#endif
static Hz5LargeRawNode* g_hz5_largefront_source_free[HZ5_LARGEFRONT_CLASS_COUNT];
static pthread_mutex_t g_hz5_largefront_source_lock =
    PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_hz5_largefront_map_lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local Hz5LargeTls g_hz5_largefront_tls;

static void hz5_largefront_mark_orphan(Hz5LargeSpan* span);
static int hz5_largefront_state_cas(Hz5LargeSpan* span,
                                    unsigned char from,
                                    unsigned char to);
static int hz5_largefront_remote_pending_to_local(Hz5LargeSpan* span);

#if BENCHLAB_HZ5_LINUX_LARGEFRONT_ADAPTIVE128 || \
    BENCHLAB_HZ5_LINUX_LARGEFRONT_POLICY_L1A
// Diagnostic-only L3 policy state. This is intentionally updated only while
// holding the source lock; do not move it into malloc/free hot paths.
static size_t g_hz5_largefront_mapped_spans[HZ5_LARGEFRONT_CLASS_COUNT];
#endif
