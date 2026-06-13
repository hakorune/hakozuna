#ifndef H7_SPAN_CLASS_MAX
#define H7_SPAN_CLASS_MAX 16384u
#endif
#define H7_DIRECT_MIN (H7_SPAN_CLASS_MAX + 1u)
#define H7_SPAN_BYTES (64u * 1024u)
#ifndef H7_EMPTY_SPAN_CAP
#define H7_EMPTY_SPAN_CAP 4u
#endif
#define H7_DIRECT_RETAIN_32K (32u * 1024u)
#define H7_DIRECT_RETAIN_64K (64u * 1024u)
#define H7_DIRECT_RETAIN_BUCKET_COUNT 2u
#if defined(H7_REMOTE_NATURAL_PRESET) && !defined(H7_ROUTE_CAPACITY)
#define H7_ROUTE_CAPACITY 16384u
#endif
#ifndef H7_DIRECT_RETAIN_CAP
#define H7_DIRECT_RETAIN_CAP 64u
#endif
#define H7_FREE_NONE UINT32_MAX
#define H7_ROUTE_SLOT_NONE UINT32_MAX
#define H7_MAGIC 0x48375A37u
#define H7_COOKIE 0x7A17C0DEu
#define H7_REGION_ACTIVE 0x1u
#define H7_REGION_RETAINED 0x2u
#ifndef H7_ROUTE_CAPACITY
#define H7_ROUTE_CAPACITY 4096u
#endif

_Static_assert((H7_SPAN_BYTES & (H7_SPAN_BYTES - 1u)) == 0,
               "H7_SPAN_BYTES must be a power of two");
_Static_assert((H7_ROUTE_CAPACITY & (H7_ROUTE_CAPACITY - 1u)) == 0,
               "H7_ROUTE_CAPACITY must be a power of two");

typedef enum H7RegionKind {
  H7_REGION_SMALL_SPAN = 1,
  H7_REGION_DIRECT = 2
} H7RegionKind;

typedef struct H7RegionHeader {
  uint32_t magic;
  uint32_t cookie;
  uint16_t kind;
  uint16_t flags;
  uint32_t reserved;
  size_t region_size;
} H7RegionHeader;

typedef struct H7Span {
  H7RegionHeader region;
  uint16_t class_id;
  uint16_t span_flags;
  uint32_t slot_size;
  uint32_t slot_count;
  uint32_t used_count;
  uint32_t free_head;
  uint32_t bitmap_words;
  uint32_t slot_offset;
  struct H7Span* next;
  struct H7Span* prev;
} H7Span;

typedef struct H7Direct {
  H7RegionHeader region;
  size_t requested_size;
} H7Direct;

typedef struct H7Class {
  uint32_t slot_size;
  H7Span* partial;
  H7Span* empty;
  uint32_t empty_count;
} H7Class;

typedef struct H7DirectRetainBucket {
  size_t max_size;
  H7Direct* items[H7_DIRECT_RETAIN_CAP];
  uint32_t count;
} H7DirectRetainBucket;

typedef struct H7RouteEntry {
  uintptr_t base;
  size_t size;
  H7RegionKind kind;
  uint16_t active;
} H7RouteEntry;

typedef struct H7RouteResult {
  H7RouteKind kind;
  H7RegionHeader* region;
} H7RouteResult;

typedef struct H7PendingRelease {
  void* ptr;
  size_t size;
} H7PendingRelease;

static H7Class g_h7_classes[] = {
    {16u, 0, 0, 0},   {32u, 0, 0, 0},   {64u, 0, 0, 0},
    {128u, 0, 0, 0},  {256u, 0, 0, 0},  {512u, 0, 0, 0},
    {1024u, 0, 0, 0}, {2048u, 0, 0, 0}, {4096u, 0, 0, 0},
    {8192u, 0, 0, 0}, {16384u, 0, 0, 0},
#if H7_SPAN_CLASS_MAX >= 32768u
    {32768u, 0, 0, 0},
#endif
};

static H7Stats g_h7_stats;
static H7RouteEntry g_h7_routes[H7_ROUTE_CAPACITY];
static H7DirectRetainBucket g_h7_direct_retain[H7_DIRECT_RETAIN_BUCKET_COUNT] = {
    {H7_DIRECT_RETAIN_32K, {0}, 0},
    {H7_DIRECT_RETAIN_64K, {0}, 0},
};

#ifdef _WIN32
static volatile LONG g_h7_lock;
#else
static volatile int g_h7_lock;
#endif
