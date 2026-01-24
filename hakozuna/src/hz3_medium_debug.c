#define _GNU_SOURCE

#include "hz3_medium_debug.h"

#if HZ3_S72_MEDIUM_DEBUG

#include "hz3_arena.h"
#include "hz3_tcache.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static _Atomic int g_s72_medium_shot = 0;

static inline int hz3_s72_medium_should_log(void) {
#if HZ3_S72_MEDIUM_SHOT
    return (atomic_exchange_explicit(&g_s72_medium_shot, 1, memory_order_relaxed) == 0);
#else
    return 1;
#endif
}

static void hz3_s72_medium_report(const char* where,
                                  const char* why,
                                  void* ptr,
                                  int sc,
                                  int owner,
                                  uint32_t page_idx,
                                  uint16_t tag,
                                  int tag_kind,
                                  int tag_sc,
                                  int tag_owner,
                                  uint32_t want_bin,
                                  uint32_t tag32,
                                  uint32_t bin,
                                  uint8_t dst) {
    if (hz3_s72_medium_should_log()) {
#if HZ3_S88_PTAG32_CLEAR_TRACE
        uint64_t last_seq = 0;
        uint32_t last_page_idx = 0;
        uint32_t last_old_tag = 0;
        void* last_ra = NULL;
        uintptr_t last_base = 0;
        unsigned long last_off = 0;
        int have_last = hz3_ptag32_clear_last_snapshot(&last_seq,
                                                      &last_page_idx,
                                                      &last_old_tag,
                                                      &last_ra,
                                                      &last_base,
                                                      &last_off);
        uint32_t last_old_bin = (have_last && last_old_tag) ? hz3_pagetag32_bin(last_old_tag) : 0;
        uint8_t last_old_dst = (have_last && last_old_tag) ? hz3_pagetag32_dst(last_old_tag) : 0;
        int last_match = (have_last && last_page_idx == page_idx) ? 1 : 0;
#endif
#if HZ3_S92_PTAG32_STORE_LAST
        uint64_t last_store_seq = 0;
        uint32_t last_store_page_idx = 0;
        uint32_t last_store_old_tag = 0;
        uint32_t last_store_new_tag = 0;
        void* last_store_ra = NULL;
        uintptr_t last_store_base = 0;
        unsigned long last_store_off = 0;
        int have_store = hz3_ptag32_store_last_snapshot(&last_store_seq,
                                                       &last_store_page_idx,
                                                       &last_store_old_tag,
                                                       &last_store_new_tag,
                                                       &last_store_ra,
                                                       &last_store_base,
                                                       &last_store_off);
        uint32_t last_store_new_bin = (have_store && last_store_new_tag) ? hz3_pagetag32_bin(last_store_new_tag) : 0;
        uint8_t last_store_new_dst = (have_store && last_store_new_tag) ? hz3_pagetag32_dst(last_store_new_tag) : 0;
        int last_store_match = (have_store && last_store_page_idx == page_idx) ? 1 : 0;
#endif
#if HZ3_S98_PTAG32_CLEAR_MAP
        uint64_t map_seq = 0;
        uint32_t map_old_tag = 0;
        void* map_ra = NULL;
        uintptr_t map_base = 0;
        unsigned long map_off = 0;
        int map_hit = hz3_ptag32_clear_map_lookup(page_idx, &map_seq, &map_old_tag, &map_ra, &map_base, &map_off);
        uint32_t map_old_bin = (map_hit && map_old_tag) ? hz3_pagetag32_bin(map_old_tag) : 0;
        uint8_t map_old_dst = (map_hit && map_old_tag) ? hz3_pagetag32_dst(map_old_tag) : 0;
#endif
        fprintf(stderr,
                "[HZ3_S72_MEDIUM] where=%s why=%s ptr=%p sc=%d owner=%d page_idx=%u "
                "tag=0x%x kind=%d tag_sc=%d tag_owner=%d want_bin=%u tag32=0x%x bin=%u dst=%u "
#if HZ3_S88_PTAG32_CLEAR_TRACE
                "last_clear_seq=%llu last_clear_match=%d last_clear_page_idx=%u last_clear_old=0x%x "
                "last_clear_old_bin=%u last_clear_old_dst=%u last_clear_off=0x%lx "
#endif
#if HZ3_S92_PTAG32_STORE_LAST
                "last_store_seq=%llu last_store_match=%d last_store_page_idx=%u "
                "last_store_new=0x%x last_store_new_bin=%u last_store_new_dst=%u last_store_off=0x%lx "
#endif
#if HZ3_S98_PTAG32_CLEAR_MAP
                "clear_map_hit=%d clear_map_seq=%llu clear_map_old=0x%x clear_map_old_bin=%u clear_map_old_dst=%u clear_map_off=0x%lx "
#endif
                "sub4k=%d medium_base=%u bin_total=%u\n",
                where, why, ptr, sc, owner, page_idx, (unsigned)tag,
                tag_kind, tag_sc, tag_owner,
                want_bin,
                tag32, bin, (unsigned)dst,
#if HZ3_S88_PTAG32_CLEAR_TRACE
                (unsigned long long)last_seq,
                last_match,
                last_page_idx,
                last_old_tag,
                last_old_bin,
                (unsigned)last_old_dst,
                last_off,
#endif
#if HZ3_S92_PTAG32_STORE_LAST
                (unsigned long long)last_store_seq,
                last_store_match,
                last_store_page_idx,
                last_store_new_tag,
                last_store_new_bin,
                (unsigned)last_store_new_dst,
                last_store_off,
#endif
#if HZ3_S98_PTAG32_CLEAR_MAP
                map_hit,
                (unsigned long long)map_seq,
                map_old_tag,
                map_old_bin,
                (unsigned)map_old_dst,
                map_off,
#endif
                (int)HZ3_SUB4K_ENABLE, (unsigned)HZ3_MEDIUM_BIN_BASE, (unsigned)HZ3_BIN_TOTAL);
    }
#if HZ3_S72_MEDIUM_FAILFAST
    abort();
#endif
}

void hz3_medium_boundary_check_ptr(const char* where, void* ptr, int sc, int owner) {
    if (!ptr) {
        hz3_s72_medium_report(where, "ptr_null", ptr, sc, owner,
                              0, 0, 0, 0, 0, 0, 0, 0, 0);
        return;
    }
    if (sc < 0 || sc >= HZ3_NUM_SC) {
        hz3_s72_medium_report(where, "sc_oob", ptr, sc, owner,
                              0, 0, 0, 0, 0, 0, 0, 0, 0);
        return;
    }
    if (owner < 0 || owner >= (int)HZ3_NUM_SHARDS) {
        hz3_s72_medium_report(where, "owner_oob", ptr, sc, owner,
                              0, 0, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    uint32_t page_idx = 0;
    if (!hz3_arena_page_index_fast(ptr, &page_idx)) {
        hz3_s72_medium_report(where, "out_of_arena", ptr, sc, owner,
                              0, 0, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

#if HZ3_PTAG_DSTBIN_ENABLE
    if (g_hz3_page_tag32) {
        uint32_t tag32 = hz3_pagetag32_load(page_idx);
        uint32_t want_bin = (uint32_t)hz3_bin_index_medium(sc);
        if (tag32 == 0) {
            hz3_s72_medium_report(where, "ptag32_zero", ptr, sc, owner,
                                  page_idx, 0, 0, 0, 0, want_bin, tag32, 0, 0);
            return;
        }
        uint32_t bin = hz3_pagetag32_bin(tag32);
        uint8_t dst = hz3_pagetag32_dst(tag32);
        if (bin != want_bin || dst != (uint8_t)owner) {
            hz3_s72_medium_report(where, "ptag32_mismatch", ptr, sc, owner,
                                  page_idx, 0, 0, 0, 0, want_bin, tag32, bin, dst);
            return;
        }
    }
#endif

#if HZ3_PTAG16_DISPATCH_ENABLE
    if (g_hz3_page_tag) {
        uint16_t tag = hz3_pagetag_load(page_idx);
        int tag_sc = -1;
        int tag_owner = -1;
        int kind = -1;
        hz3_pagetag_decode_with_kind(tag, &tag_sc, &tag_owner, &kind);
        if (kind != PTAG_KIND_V1_MEDIUM || tag_sc != sc || tag_owner != owner) {
            hz3_s72_medium_report(where, "ptag_mismatch", ptr, sc, owner,
                                  page_idx, tag, kind, tag_sc, tag_owner, 0, 0, 0, 0);
            return;
        }
    }
#endif
}

#endif  // HZ3_S72_MEDIUM_DEBUG
