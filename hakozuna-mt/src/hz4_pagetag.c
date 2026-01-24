// hz4_pagetag.c - HZ4 Page Tag Table Implementation
// Box Theory: 初期化と global 変数の実体定義
#define _GNU_SOURCE
#include "hz4_pagetag.h"

#if HZ4_PAGE_TAG_TABLE

#include <sys/mman.h>
#include <stddef.h>

// Global variables (extern declared in hz4_pagetag.h)
_Atomic(uint32_t)* g_hz4_page_tag = NULL;
void* g_hz4_arena_base = NULL;

void hz4_pagetag_init(void* arena_base) {
    // Already initialized check
    if (g_hz4_arena_base) return;

    g_hz4_arena_base = arena_base;

    // Allocate tag table via mmap
    // Size: HZ4_MAX_PAGES * sizeof(_Atomic(uint32_t))
    // For 4GB arena with 64KB pages: 64K * 4 = 256KB
    size_t table_size = HZ4_MAX_PAGES * sizeof(_Atomic(uint32_t));

    void* mem = mmap(NULL, table_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED) {
        g_hz4_page_tag = NULL;
        return;
    }

    // mmap returns zero-initialized pages (tag=0 means unknown)
    g_hz4_page_tag = (_Atomic(uint32_t)*)mem;

    // Optional: THP hint for random access improvement
    // madvise(mem, table_size, MADV_HUGEPAGE);
}

#endif // HZ4_PAGE_TAG_TABLE
