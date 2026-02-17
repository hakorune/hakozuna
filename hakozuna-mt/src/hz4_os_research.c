// hz4_os_research.c - HZ4 OS Research Box helpers
// Box Theory: research-only knobs are isolated here to avoid #if scattering.

#include <sys/mman.h>

#include "hz4_config.h"
#include "hz4_os.h"

int hz4_os_mmap_seg_extra_flags(void) {
    int extra = 0;
#if HZ4_SEG_MMAP_POPULATE && defined(MAP_POPULATE)
    extra |= MAP_POPULATE;
#endif
    return extra;
}

void hz4_os_seg_research_advise(void* seg_base) {
    if (!seg_base) {
        return;
    }

    // K1: SegPopulateBox
#if HZ4_SEG_MADVISE_POPULATE_WRITE && defined(MADV_POPULATE_WRITE)
    (void)madvise(seg_base, (size_t)HZ4_SEG_SIZE, MADV_POPULATE_WRITE);
#endif

    // K2: THP Hint Box
#if HZ4_SEG_MADVISE_HUGEPAGE && defined(MADV_HUGEPAGE)
    (void)madvise(seg_base, (size_t)HZ4_SEG_SIZE, MADV_HUGEPAGE);
#endif

#if HZ4_SEG_MADVISE_NOHUGEPAGE && defined(MADV_NOHUGEPAGE)
    (void)madvise(seg_base, (size_t)HZ4_SEG_SIZE, MADV_NOHUGEPAGE);
#endif
}

