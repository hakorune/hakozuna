#ifndef HZ6_PRELOAD_MIDPAGE_H
#define HZ6_PRELOAD_MIDPAGE_H

#include "hz6_allocator.h"

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_MIDPAGE_INTERNAL __attribute__((visibility("hidden")))
#else
#define HZ6_PRELOAD_MIDPAGE_INTERNAL
#endif

#if HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1 ||   \
    HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1 || \
    HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1
#define HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1 1
#else
#define HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1 0
#endif

#if HZ6_PRELOAD_MIDPAGE_MALLOC_BOUNDARY_NOINLINE_L1
#if defined(__GNUC__) || defined(__clang__)
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define HZ6_PRELOAD_MIDPAGE_BOUNDARY_UNLIKELY(expr) (expr)
#endif

HZ6_PRELOAD_MIDPAGE_INTERNAL void*
hz6_preload_malloc_midpage_boundary(Hz6Allocator* allocator, size_t size);
#endif

#if HZ6_PRELOAD_FREE_MIDPAGE_HINT_ACTIVE_L1
HZ6_PRELOAD_MIDPAGE_INTERNAL void
hz6_preload_midpage_hint_note(const void* ptr, size_t size);
HZ6_PRELOAD_MIDPAGE_INTERNAL int
hz6_preload_midpage_hint_maybe(const void* ptr);
#else
#define hz6_preload_midpage_hint_note(ptr, size) ((void)0)
#define hz6_preload_midpage_hint_maybe(ptr) (0)
#endif

#endif
