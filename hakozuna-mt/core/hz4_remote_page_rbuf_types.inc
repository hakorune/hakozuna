// hz4_remote_page_rbuf_types.inc - RemotePageRbufBox type/state slice
#ifndef HZ4_REMOTE_PAGE_RBUF_TYPES_INC
#define HZ4_REMOTE_PAGE_RBUF_TYPES_INC

// Per-owner global pageq (per sc; avoids bucket collision churn).
static _Atomic(hz4_page_t*) g_hz4_remote_page_rbufq[HZ4_NUM_SHARDS][HZ4_SC_MAX];

// notify implementation is defined in the parent unit.
static inline void hz4_remote_page_rbufq_notify(uint8_t owner, uint8_t sc,
                                                hz4_page_t* page, hz4_page_meta_t* meta);

#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
typedef struct hz4_remote_idx_msg_slot {
    _Alignas(64) _Atomic(uint8_t) ready;
    uint8_t _pad0[3];
    uint32_t next_code;
    uint8_t sc;
    uint8_t _pad1;
    uint16_t n;
    hz4_page_t* page;
    uint16_t idx[HZ4_REMOTE_PAGE_STAGING_MAX];
} hz4_remote_idx_msg_slot_t;

static hz4_remote_idx_msg_slot_t
    g_hz4_remote_idx_msg_slots[HZ4_NUM_SHARDS][HZ4_NUM_SHARDS];
static _Atomic(uint32_t) g_hz4_remote_idx_msg_heads[HZ4_NUM_SHARDS][HZ4_SC_MAX];
#endif

#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
typedef struct hz4_remote_idx_msg_node {
    struct hz4_remote_idx_msg_node* next;
    hz4_page_t* page;
    uint16_t n;
    uint8_t sc;
    uint8_t _pad;
    uint16_t idx[HZ4_REMOTE_PAGE_STAGING_MAX];
} hz4_remote_idx_msg_node_t;

static hz4_remote_idx_msg_node_t
    g_hz4_remote_idx_msg_nodes[HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N];
static _Atomic(hz4_remote_idx_msg_node_t*) g_hz4_remote_idx_msg_free;
static _Atomic(hz4_remote_idx_msg_node_t*) g_hz4_remote_idx_msgq[HZ4_NUM_SHARDS][HZ4_SC_MAX];
static _Atomic(uint8_t) g_hz4_remote_idx_msg_pool_ready;
static atomic_flag g_hz4_remote_idx_msg_pool_lock = ATOMIC_FLAG_INIT;
static _Thread_local hz4_remote_idx_msg_node_t* t_hz4_remote_idx_msg_cache;
static _Thread_local uint16_t t_hz4_remote_idx_msg_cache_n;
#endif

#endif // HZ4_REMOTE_PAGE_RBUF_TYPES_INC
