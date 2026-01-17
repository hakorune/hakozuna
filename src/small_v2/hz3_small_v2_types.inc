// hz3_small_v2_types.inc - Types, constants, and static globals
// Part of hz3_small_v2.c (single TU split)

// Task 3: page_hdr に magic/owner/sc を集約して seg_hdr cold reads を除去
#if HZ3_S121_PAGE_LOCAL_REMOTE
#define HZ3_SMALL_V2_PAGE_HDR_SIZE 32u  // S121: extended header
#else
#define HZ3_SMALL_V2_PAGE_HDR_SIZE 16u
#endif
#define HZ3_PAGE_MAGIC 0x485A5032u  // "HZP2"

// static_assert: sc を uint8_t で保持するため
_Static_assert(HZ3_SMALL_NUM_SC <= 255, "sc must fit in uint8_t");

typedef struct {
    uint32_t magic;    // HZ3_PAGE_MAGIC で small v2 ページを識別
    uint8_t  owner;    // seg_hdr から複製（ページ割当時）
    uint8_t  sc;       // size class
#if HZ3_S69_LIVECOUNT
    _Atomic uint16_t live_count;  // S69: objects held by application (not in allocator caches)
#else
    uint16_t flags;    // reserved
#endif

#if HZ3_S121_PAGE_LOCAL_REMOTE
    // S121: Page-local remote fields (24B extension → total 32B)
    _Atomic(void*)    remote_head;   // intrusive object list (MPSC)
    _Atomic(uint8_t)  remote_state;  // 0=idle, 1=queued_or_processing
    _Atomic(uint8_t)  remote_count;  // S121-A: lazy enqueue counter
    uint8_t           pad[6];        // alignment padding
    void*             page_qnext;    // worklist node link (non-atomic, owner-only)
#else
    uint64_t reserved; // パディング（16B境界）
#endif
} Hz3SmallV2PageHdr;
