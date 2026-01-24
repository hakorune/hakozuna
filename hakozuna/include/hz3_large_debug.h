#pragma once

#include "hz3_config.h"
#include "hz3_large_internal.h"

#include <stddef.h>
#include <stdint.h>

#define HZ3_LARGE_CANARY_SIZE 8u

#if HZ3_LARGE_DEBUG

void hz3_large_debug_on_bucket_insert_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_on_bucket_remove_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_on_cache_insert_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_on_cache_remove_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_on_munmap_locked(Hz3LargeHdr* hdr);

void hz3_large_debug_check_bucket_node_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_bad_hdr_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_bad_size_locked(Hz3LargeHdr* hdr);
void hz3_large_debug_bad_inuse(const Hz3LargeHdr* hdr);

void hz3_large_debug_write_canary(const Hz3LargeHdr* hdr);
int hz3_large_debug_check_canary_free(const Hz3LargeHdr* hdr);
int hz3_large_debug_check_canary_cache(const Hz3LargeHdr* hdr, int aligned);

#else

static inline void hz3_large_debug_on_bucket_insert_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_on_bucket_remove_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_on_cache_insert_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_on_cache_remove_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_on_munmap_locked(Hz3LargeHdr* hdr) { (void)hdr; }

static inline void hz3_large_debug_check_bucket_node_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_bad_hdr_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_bad_size_locked(Hz3LargeHdr* hdr) { (void)hdr; }
static inline void hz3_large_debug_bad_inuse(const Hz3LargeHdr* hdr) { (void)hdr; }

static inline void hz3_large_debug_write_canary(const Hz3LargeHdr* hdr) { (void)hdr; }
static inline int hz3_large_debug_check_canary_free(const Hz3LargeHdr* hdr) { (void)hdr; return 1; }
static inline int hz3_large_debug_check_canary_cache(const Hz3LargeHdr* hdr, int aligned) {
    (void)hdr;
    (void)aligned;
    return 1;
}

#endif
