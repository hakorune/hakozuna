#if HZ5_LOWPAGE64_P43_SEGMENT_SLOTS
#if HZ5_LOWPAGE64_P43_SLOT_COUNT > 32u
#error "P43 segment slot bitmap currently supports up to 32 slots"
#endif

#define HZ5_LOWPAGE64_P43_COMMITTED_LISTS \
  (HZ5_LOWPAGE64_P43_DESCRIPTOR_LISTS || \
   HZ5_LOWPAGE64_P43_COMMITTED_RETAIN_CAP > 0u || \
   HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u || \
   HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u)

typedef struct Hz5Lowpage64Segment Hz5Lowpage64Segment;

typedef struct Hz5Lowpage64P43SlotRef {
  Hz5Lowpage64Segment* seg;
  uint32_t slot;
} Hz5Lowpage64P43SlotRef;

struct Hz5Lowpage64Segment {
  void* base;
  _Atomic uint32_t allocated_mask;
  _Atomic uint32_t committed_mask;
  _Atomic uint32_t cold_mask;
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u
  _Atomic uint32_t pending_free_mask;
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS
  Hz5Lowpage64P43SlotRef free_next[HZ5_LOWPAGE64_P43_SLOT_COUNT];
#endif
  Hz5Lowpage64Segment* next;
};

#if defined(_WIN32)
static INIT_ONCE g_hz5_lowpage64_p43_lock_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION g_hz5_lowpage64_p43_lock;
#else
static pthread_mutex_t g_hz5_lowpage64_p43_lock = PTHREAD_MUTEX_INITIALIZER;
#endif
static Hz5Lowpage64Segment* g_hz5_lowpage64_p43_segments;
#if HZ5_LOWPAGE64_P43_FAST_LOOKUP
static _Atomic(Hz5Lowpage64Segment*)
    g_hz5_lowpage64_p43_fast_lookup[HZ5_LOWPAGE64_P43_FAST_LOOKUP_CAP];
#endif
#if HZ5_LOWPAGE64_P43_COMMITTED_LISTS && !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
static Hz5Lowpage64P43SlotRef g_hz5_lowpage64_p43_committed_head;
static size_t g_hz5_lowpage64_p43_committed_count;
#endif
#if HZ5_LOWPAGE64_P43_TLS_CACHE_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
typedef struct Hz5Lowpage64P43TlsCache {
  Hz5Lowpage64P43SlotRef refs[HZ5_LOWPAGE64_P43_TLS_CACHE_CAP];
  uint32_t count;
} Hz5Lowpage64P43TlsCache;

#if defined(_WIN32)
__declspec(thread) static Hz5Lowpage64P43TlsCache
    g_hz5_lowpage64_p43_tls_cache;
#else
static __thread Hz5Lowpage64P43TlsCache g_hz5_lowpage64_p43_tls_cache;
#endif
#endif
#if HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP > 0u && \
    !HZ5_LOWPAGE64_P43_PAGE_NOACCESS
typedef struct Hz5Lowpage64P43ReleaseBuffer {
  Hz5Lowpage64P43SlotRef refs[HZ5_LOWPAGE64_P43_RELEASE_BUFFER_CAP];
  uint32_t count;
} Hz5Lowpage64P43ReleaseBuffer;

#if defined(_WIN32)
__declspec(thread) static Hz5Lowpage64P43ReleaseBuffer
    g_hz5_lowpage64_p43_release_buffer;
#else
static __thread Hz5Lowpage64P43ReleaseBuffer
    g_hz5_lowpage64_p43_release_buffer;
#endif
#endif
#endif

#if HZ5_LOWPAGE64_STATS
static _Atomic size_t g_hz5_lowpage64_p43_segments_reserved;
static _Atomic size_t g_hz5_lowpage64_p43_segments_released;
static _Atomic size_t g_hz5_lowpage64_p43_slot_commits;
static _Atomic size_t g_hz5_lowpage64_p43_slot_commit_failures;
static _Atomic size_t g_hz5_lowpage64_p43_slot_decommits;
static _Atomic size_t g_hz5_lowpage64_p43_slot_decommit_failures;
static _Atomic size_t g_hz5_lowpage64_p43_tls_hits;
static _Atomic size_t g_hz5_lowpage64_p43_tls_pushes;
static _Atomic size_t g_hz5_lowpage64_p43_tls_overflow_pushes;
static _Atomic size_t g_hz5_lowpage64_p43_global_hits;
static _Atomic size_t g_hz5_lowpage64_p43_global_pushes;
static _Atomic size_t g_hz5_lowpage64_p43_cold_hits;
static _Atomic size_t g_hz5_lowpage64_p43_free_slot_hits;
static _Atomic size_t g_hz5_lowpage64_p43_source_alloc_calls;
static _Atomic size_t g_hz5_lowpage64_p43_source_cold_segment_scans_total;
static _Atomic size_t g_hz5_lowpage64_p43_source_cold_segment_scans_max;
static _Atomic size_t g_hz5_lowpage64_p43_source_free_segment_scans_total;
static _Atomic size_t g_hz5_lowpage64_p43_source_free_segment_scans_max;
static _Atomic size_t g_hz5_lowpage64_p43_source_new_segment_scan_total;
static _Atomic size_t g_hz5_lowpage64_p43_source_new_segment_scan_max;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_calls;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_active;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_nonactive;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_miss;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_segments_scanned_total;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_segments_scanned_max;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_fast_hits;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_fast_misses;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_lockless_hits;
static _Atomic size_t g_hz5_lowpage64_p43_lookup_lockless_misses;
static _Atomic size_t g_hz5_lowpage64_p43_find_fast_hits;
static _Atomic size_t g_hz5_lowpage64_p43_find_segments_scanned_total;
static _Atomic size_t g_hz5_lowpage64_p43_find_segments_scanned_max;
static _Atomic size_t g_hz5_lowpage64_p43_va_allocs;
static _Atomic size_t g_hz5_lowpage64_p43_va_alloc_failures;
static _Atomic size_t g_hz5_lowpage64_p43_va_releases;
#endif
