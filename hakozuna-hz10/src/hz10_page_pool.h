#ifndef HZ10_PAGE_POOL_H
#define HZ10_PAGE_POOL_H

#include <stdint.h>

/*
 * HZ10BoundedPagePool-L0 (Box 4), part 1 of 2: a generic, bounded cache of
 * raw HZ10_PAGE_QUANTUM-aligned, HZ10_PAGE_QUANTUM-sized blocks. Knows
 * nothing about Hz10FreelistPage, size classes, or pagemap registration --
 * it only recycles raw memory, so any future box (multi-class, other page
 * shapes) can share the same pool. src/hz10_pooled_page.{h,c} is the glue
 * that ties this to Box 2's freelist page.
 *
 * Design rule (RSS Contract, docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md):
 * "global: bounded page pool, cap overflow returns/decommits pages." A
 * mutex-protected stack is used on purpose instead of a lock-free Treiber
 * stack -- acquire (pop) and release (push) both happen from arbitrary
 * threads here (unlike Box 3's remote_free_head, which is push-only from
 * foreign threads and drained via a single atomic exchange), and a
 * concurrent pop while another thread's block is mid-release is exactly
 * the classic Treiber-stack ABA hazard. Acquire/release happen once per
 * page lifecycle, not once per malloc/free, so mutex contention here is
 * not the hot-path concern Box 2/3's atomics were built to avoid.
 */

#define HZ10_PAGE_POOL_DEFAULT_CAP 64u

/* Pops a cached block if one is available. Returns NULL if the pool is
 * empty (caller should mmap a fresh block itself). */
void* hz10_page_pool_try_acquire(void);

/*
 * Offers base (a HZ10_PAGE_QUANTUM block the caller no longer needs) back
 * to the pool. If the pool is under its cap, base is cached and this
 * returns 1. If the pool is already at cap, base is actually released
 * (hz10_platform_release) here and this returns 0 -- the caller must not
 * touch base again either way.
 */
int hz10_page_pool_release(void* base);

/* Sets the cap (max cached blocks) and returns the previous value. Mainly
 * for tests; a lower cap does not evict already-cached blocks immediately,
 * it only affects future release() calls. */
uint32_t hz10_page_pool_set_cap(uint32_t cap);

uint32_t hz10_page_pool_cached_count(void);
uint64_t hz10_page_pool_reuse_count(void);   /* successful acquire-from-pool */
uint64_t hz10_page_pool_release_count(void); /* real hz10_platform_release calls */

/* Test/bench only: releases every currently cached block for real and
 * resets all counters and the cap to HZ10_PAGE_POOL_DEFAULT_CAP. */
void hz10_page_pool_reset_for_tests(void);

#endif
