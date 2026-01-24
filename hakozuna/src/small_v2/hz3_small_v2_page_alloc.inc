// hz3_small_v2_page_alloc.inc - Page allocation for small objects
// Part of hz3_small_v2.c (single TU split)

#if HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE

static void* hz3_small_v2_alloc_page(int sc) {
    // CollisionGuardBox: small segment header (free_bits/free_pages) is "my_shard-only".
    // When shard collision is tolerated (threads > shards), multiple writers can race here.
    uint8_t my_shard = (uint8_t)t_hz3_cache.my_shard;

    // Acquire lease + excl without holding OwnerLease while waiting on OwnerExcl.
    // - OwnerLease hold_us_max is used as a tail-latency SSOT.
    // - Keep the hold window tight under oversubscription.
    Hz3OwnerLeaseToken lease_token;
    Hz3OwnerExclToken excl_token;
    for (;;) {
        lease_token = hz3_owner_lease_try_acquire(my_shard);
        if (!lease_token.active) {
            sched_yield();
            continue;
        }

        excl_token = hz3_owner_excl_try_acquire(my_shard);
        if (excl_token.active) {
            break;
        }

        hz3_owner_lease_release(lease_token);
        sched_yield();
    }
    int collision_active = hz3_small_v2_collision_active();

    Hz3SegHdr* hdr = hz3_small_v2_current_hdr_get(collision_active);
    if (!hdr || hdr->free_pages == 0) {
        hdr = hz3_seg_alloc_small(my_shard);
        if (!hdr) {
#if HZ3_OOM_SHOT
            hz3_small_v2_oom_dump(sc);
#endif
            hz3_owner_excl_release(excl_token);
            hz3_owner_lease_release(lease_token);
            return NULL;
        }
        hz3_small_v2_current_hdr_set(hdr, collision_active);
    }

    int start_page = hz3_bitmap_find_free(hdr->free_bits, 1);
    if (start_page < 0) {
        hdr = hz3_seg_alloc_small(my_shard);
        if (!hdr) {
#if HZ3_OOM_SHOT
            hz3_small_v2_oom_dump(sc);
#endif
            hz3_owner_excl_release(excl_token);
            hz3_owner_lease_release(lease_token);
            return NULL;
        }
        hz3_small_v2_current_hdr_set(hdr, collision_active);
        start_page = hz3_bitmap_find_free(hdr->free_bits, 1);
        if (start_page < 0) {
#if HZ3_OOM_SHOT
            hz3_small_v2_oom_dump(sc);
#endif
            hz3_owner_excl_release(excl_token);
            hz3_owner_lease_release(lease_token);
            return NULL;
        }
    }

    hz3_bitmap_mark_used(hdr->free_bits, (size_t)start_page, 1);
    if (hdr->free_pages > 0) {
        hdr->free_pages--;
    }

    char* page_base = (char*)hdr + ((size_t)start_page << HZ3_PAGE_SHIFT);
#if HZ3_S62_RETIRE_MPROTECT
    // If S62 retired this page, it may be PROT_NONE. Unprotect before touching.
    if (mprotect(page_base, HZ3_PAGE_SIZE, PROT_READ | PROT_WRITE) != 0) {
        fprintf(stderr,
                "[HZ3_S62_RETIRE_UNPROTECT] mprotect(PROT_RW) failed: page=%p errno=%d\n",
                page_base, errno);
        abort();
    }
#endif
    Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base;
    ph->magic = HZ3_PAGE_MAGIC;
    ph->owner = hdr->owner;  // seg_hdr から複製
    ph->sc = (uint8_t)sc;
#if HZ3_S69_LIVECOUNT
    atomic_init(&ph->live_count, 0);
#else
    ph->flags = 0;
#endif
#if HZ3_S121_PAGE_LOCAL_REMOTE
    atomic_init(&ph->remote_head, NULL);
    atomic_init(&ph->remote_state, 0);
    atomic_init(&ph->remote_count, 0);  // S121-A: lazy enqueue counter
    ph->page_qnext = NULL;
#elif HZ3_PAGE_REMOTE_HEADER
    for (uint32_t i = 0; i < HZ3_PAGE_REMOTE_SHARDS; i++) {
        atomic_init(&ph->remote_head[i], NULL);
    }
#if HZ3_LANE16_REMOTE_LOCK
    for (uint32_t i = 0; i < HZ3_PAGE_REMOTE_SHARDS; i++) {
        atomic_init(&ph->remote_lock[i], 0);
    }
#endif
#if HZ3_LANE16_DUP_FAILFAST || HZ3_LANE16_DUP_GUARD
    for (uint32_t i = 0; i < (uint32_t)HZ3_LANE16_DUP_WORDS; i++) {
        atomic_init(&ph->lane16_dup_bits[i], 0);
    }
#endif
#else
    ph->reserved = 0;
#endif

#if HZ3_SMALL_V2_PTAG_ENABLE || HZ3_PTAG_DSTBIN_ENABLE
    uint32_t page_idx;
    if (hz3_arena_page_index_fast(page_base, &page_idx)) {
#if HZ3_SMALL_V2_PTAG_ENABLE
        // S12-4B: event-only tag set (page allocation boundary)
        hz3_pagetag_set(page_idx, sc, (int)hdr->owner);
#endif
#if HZ3_PTAG_DSTBIN_ENABLE
        // S17: dst/bin direct tag set (page allocation boundary)
        int bin = hz3_bin_index_small(sc);
        if (g_hz3_page_tag32) {
            uint32_t tag = hz3_pagetag32_encode(bin, (int)hdr->owner);
            hz3_pagetag32_store(page_idx, tag);
        }
#endif
    }
#endif

#if HZ3_S110_META_ENABLE
    // S110: store bin+1 in segment header per-page metadata (store-release for S110-1 free fastpath)
    {
        int bin = hz3_bin_index_small(sc);
        atomic_store_explicit(&hdr->page_bin_plus1[start_page], (uint16_t)(bin + 1), memory_order_release);
    }
#endif

#if HZ3_OOM_SHOT
    atomic_fetch_add_explicit(&g_small_v2_page_alloc_sc[sc], 1, memory_order_relaxed);
#endif
    hz3_owner_excl_release(excl_token);
    hz3_owner_lease_release(lease_token);
    return page_base;
}

#endif  // HZ3_SMALL_V2_ENABLE && HZ3_SEG_SELF_DESC_ENABLE
