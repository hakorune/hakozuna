// S232: Large aggressive lane preset (opt-in)
// Purpose:
// - Reduce large-path mmap/munmap churn by enabling budgeted retain.
// - Keep default behavior unchanged unless explicitly enabled.

#ifndef HZ3_S232_LARGE_AGGRESSIVE
#define HZ3_S232_LARGE_AGGRESSIVE 0
#endif

#ifndef HZ3_S232_SOFT_BYTES
#define HZ3_S232_SOFT_BYTES (1024ULL << 20)  // 1GB
#endif

#ifndef HZ3_S232_HARD_BYTES
#define HZ3_S232_HARD_BYTES (2048ULL << 20)  // 2GB
#endif

#if HZ3_S232_LARGE_AGGRESSIVE
#undef HZ3_LARGE_CACHE_BUDGET
#define HZ3_LARGE_CACHE_BUDGET 1

#undef HZ3_LARGE_CACHE_SOFT_BYTES
#define HZ3_LARGE_CACHE_SOFT_BYTES HZ3_S232_SOFT_BYTES

#undef HZ3_LARGE_CACHE_HARD_BYTES
#define HZ3_LARGE_CACHE_HARD_BYTES HZ3_S232_HARD_BYTES
#endif
