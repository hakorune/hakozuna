// Global central pool (zero-initialized)
Hz3CentralBin g_hz3_central[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
static Hz3CentralBin g_hz3_central_aligned[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#endif
#if HZ3_S65_CENTRAL_COLD_ENABLE
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
typedef enum Hz3CentralColdState {
    HZ3_CENTRAL_COLD_STATE_READY = 1,
    HZ3_CENTRAL_COLD_STATE_DECOMMITTED = 2,
    HZ3_CENTRAL_COLD_STATE_RECOMMITTING = 3,
    HZ3_CENTRAL_COLD_STATE_DECOMMITTING = 4
} Hz3CentralColdState;

typedef struct Hz3CentralColdNode {
    void* run;
    struct Hz3CentralColdNode* next;
    struct Hz3CentralColdNode* prev;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    uint8_t state;
#endif
} Hz3CentralColdNode;

typedef struct {
    hz3_lock_t lock;
    Hz3CentralColdNode* ready_head;
    Hz3CentralColdNode* ready_tail;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    Hz3CentralColdNode* decommitted_head;
    uint32_t ready_count;
    uint32_t decommitted_count;
    uint32_t inflight_decommitting;
    uint32_t inflight_recommitting;
#endif
    uint32_t count;
} Hz3CentralColdExternalBin;

static Hz3CentralColdExternalBin g_hz3_central_cold_ext[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_POOL_LAZY
static _Atomic(uint32_t) g_hz3_cold_node_pool_allocated;
#else
static Hz3CentralColdNode g_hz3_cold_nodes[HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP];
#endif
static Hz3CentralColdNode* g_hz3_cold_free_nodes;
static hz3_lock_t g_hz3_cold_node_lock;
#else
static Hz3CentralBin g_hz3_central_cold[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#endif
#endif
