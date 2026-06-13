static inline int hz3_s281_retained_free_fast_route(Hz3LargeHdr* hdr) {
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && \
    HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST
    if (!hdr || (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        return 0;
    }
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_size == 0) {
        return 0;
    }

    int sc = hz3_large_sc(hdr->map_size);
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT - 1) {
        return 0;
    }

    uint8_t cur = hz3_large_s240_current_owner();
    if (cur == UINT8_MAX || hdr->owner_shard == UINT8_MAX) {
        return 0;
    }

#if HZ3_S240_LARGE_FRONT_CACHE
    if (hdr->owner_shard == cur) {
        hdr->in_use = 0;
        hdr->next = NULL;
        if (hz3_large_s240_front_push(sc, hdr)) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            hz3_large_s276_note_front_push_retained();
#endif
            hz3_large_stats_on_free_cached();
            return 1;
        }
        return 0;
    }
#endif

#if HZ3_S240_LARGE_OWNER_INBOX
    if (hdr->owner_shard != cur) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        if (hz3_large_s240_inbox_push_remote(sc, hdr, cur)) {
            hz3_large_s276_note_inbox_push_retained();
            hz3_large_stats_on_free_cached();
            return 1;
        }
#endif
        return 0;
    }
#endif
#else
    (void)hdr;
#endif
    return 0;
}

int hz3_large_free(void* ptr) {
    if (!ptr) {
        return 0;
    }

    size_t s243_free_total_start_ns = hz3_s243_residual_timing_start();
    size_t s243_free_take_start_ns = hz3_s243_residual_timing_start();
    Hz3LargeHdr* hdr = hz3_large_take(ptr);
    hz3_s243_residual_on_free_take_elapsed(s243_free_take_start_ns, hdr != NULL);
    if (!hdr) {
        hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
        return 0;
    }
    hz3_large_aligned_obs_on_free(hdr);
    hz3_s292_on_free_hdr(hdr);
    hz3_large_cache_stats_dump();
    hz3_large_stats_on_free_call();
    size_t obs_free_admit_start_ns = hz3_large_aligned_obs_timing_start();
    size_t s243_free_fallback_start_ns = 0;
    int s243_free_fallback_active = 0;
    int s280_had_fast_route = 0;
    int s280_front_reject = 0;
    int s280_inbox_reject = 0;
#if HZ3_WATCH_PTR_BOX
    hz3_watch_ptr_on_free("large_free", ptr, -1, -1);
#endif
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && (HZ3_S186_LARGE_UNMAP_DEFER || HZ3_S212_LARGE_UNMAP_DEFER_PLUS)
    hz3_large_unmap_defer_drain_budget();
#endif

#if HZ3_S281_LARGE_RETAINED_FREE_ROUTE_FAST
    if (hz3_s281_retained_free_fast_route(hdr)) {
        return 1;
    }
#endif

#if HZ3_LARGE_CACHE_ENABLE
#if HZ3_S50_LARGE_SCACHE
    // Safety: validate hdr before accessing map_size
    if (hdr->magic != HZ3_LARGE_MAGIC || hdr->map_size == 0) {
        hz3_large_aligned_obs_on_admit_invalid();
        hz3_s280_free_route_on_sc_invalid();
        if (!s243_free_fallback_active) {
            s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
            s243_free_fallback_active = 1;
        }
        goto do_munmap;
    }

    int sc = hz3_large_sc(hdr->map_size);
    hz3_s280_free_route_on_taken(hdr);
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
    Hz3LargeHdr* s270_victims[HZ3_S270_TRIM_BATCH_MAX];
    size_t s270_victim_count = 0;
#endif
    size_t s243_free_meta_start_ns = hz3_s243_residual_timing_start();
#if HZ3_S279_LARGE_DIRECT_RETAIN_SKIP_FREE_META
    if ((hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) == 0) {
        hz3_large_s240_on_free_take(hdr, sc);
    }
#else
    hz3_large_s240_on_free_take(hdr, sc);
#endif
    hz3_s243_residual_on_free_s240_meta_elapsed(s243_free_meta_start_ns);

    // >64MB はキャッシュ対象外（巨大ブロックで cap を食い潰すのを防ぐ）
    if (sc < 0 || sc >= HZ3_LARGE_SC_COUNT - 1) {
        hz3_large_aligned_obs_on_admit_oversize();
        hz3_s280_free_route_on_sc_oversize();
        if (!s243_free_fallback_active) {
            s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
            s243_free_fallback_active = 1;
        }
        goto do_munmap;
    }

#if HZ3_S240_LARGE_FRONT_CACHE
    uint8_t s240_cur_owner = hz3_large_s240_current_owner();
    if (hdr->owner_shard != UINT8_MAX && hdr->owner_shard == s240_cur_owner) {
        int s280_retained_route = (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0;
        s280_had_fast_route = 1;
        hz3_s280_free_route_on_same_owner();
        hdr->in_use = 0;
        hdr->next = NULL;
        size_t s243_front_push_start_ns = hz3_s243_residual_timing_start();
        int s243_front_pushed = hz3_large_s240_front_push(sc, hdr);
        hz3_s280_free_route_on_front_result(s280_retained_route,
                                            s243_front_pushed);
        hz3_s243_residual_on_free_front_push_elapsed(
            s243_front_push_start_ns, s243_front_pushed);
        if (s243_front_pushed) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if ((hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
                hz3_large_s276_note_front_push_retained();
            }
#endif
            hz3_large_stats_on_free_cached();
            hz3_large_aligned_obs_on_admit_cache_insert();
            hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
            hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
            return 1;
        }
        s280_front_reject = 1;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        if ((hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0) {
            hz3_large_s276_note_front_push_reject_clear();
            hz3_s276_direct_clear_retained(hdr);
        }
#endif
    }
#if HZ3_S240_LARGE_OWNER_INBOX
    if (hdr->owner_shard != UINT8_MAX && hdr->owner_shard != s240_cur_owner) {
        int s280_inbox_retained_route = 0;
        s280_had_fast_route = 1;
        hz3_s280_free_route_on_remote_owner();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        int s276_try_inbox_retain =
            (hdr->flags & HZ3_LARGE_F_DIRECT_RETAINED) != 0 &&
            HZ3_S276_LARGE_DIRECT_RETAIN_INBOX;
        s280_inbox_retained_route = s276_try_inbox_retain;
        if (!s276_try_inbox_retain) {
            hz3_s276_direct_clear_retained(hdr);
        }
#endif
        size_t s243_inbox_push_start_ns = hz3_s243_residual_timing_start();
        int s243_inbox_pushed = hz3_large_s240_inbox_push_remote(sc, hdr, s240_cur_owner);
        hz3_s280_free_route_on_inbox_result(s280_inbox_retained_route,
                                            s243_inbox_pushed);
        hz3_s243_residual_on_free_inbox_push_elapsed(
            s243_inbox_push_start_ns, s243_inbox_pushed);
        if (s243_inbox_pushed) {
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            if (s276_try_inbox_retain) {
                hz3_large_s276_note_inbox_push_retained();
            }
#endif
            hz3_large_stats_on_free_cached();
            hz3_large_aligned_obs_on_admit_cache_insert();
            hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
            hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
            return 1;
        }
        s280_inbox_reject = 1;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        if (s276_try_inbox_retain) {
            hz3_large_s276_note_inbox_push_reject_clear();
            hz3_s276_direct_clear_retained(hdr);
        }
#endif
    }
#endif
#endif
#if HZ3_S240_LARGE_OWNER_FRONT
    if (hdr->owner_shard == UINT8_MAX) {
        hz3_s280_free_route_on_unknown_owner();
    }
#endif

    if (!s243_free_fallback_active) {
        hz3_s280_free_route_on_global_fallback(s280_had_fast_route,
                                               s280_front_reject,
                                               s280_inbox_reject);
        hz3_s292_on_global_fallback(hdr);
        s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
        s243_free_fallback_active = 1;
    }

    // NOTE: If we apply madvise(MADV_DONTNEED) for the freed mapping, we MUST
    // not make it visible to other threads (cache lists) until after the
    // madvise is done. Otherwise another thread can reallocate the same mapping
    // and observe its contents being discarded while "in use".
    //
    // Therefore we:
    //   1) decide evict/can_cache + madvise range under lock
    //   2) unlock + (maybe) madvise
    //   3) lock again and insert into cache (evict again if needed)
    // S53/S207-1: hard cap (optionally widened for high-band classes).
    size_t hard_cap = hz3_large_hard_cap_for_sc(sc);
    size_t insert_cap = hz3_large_insert_cap_for_sc(sc, hard_cap);
    size_t evict_budget_left = hz3_large_evict_budget_for_sc(sc);
    int evict_budget_exhausted = 0;

    int can_cache = 0;
#if HZ3_S184_LARGE_FREE_PRECHECK
    int force_lock_admission = hz3_large_reuse_trackable_sc(sc);
#endif

    // S53: soft 判定と madvise 範囲は lock 内で計算してローカルへ退避
#if HZ3_LARGE_CACHE_BUDGET
    int do_madvise = 0;
    uintptr_t madvise_start = 0;
    size_t madvise_size = 0;
#endif

#if HZ3_S184_LARGE_FREE_PRECHECK
    // S184: lock前に relaxed hint で fast admit を先読み。
    // 最終判定は後段の lock 再取得時に必ず行う。
    if (!force_lock_admission) {
        size_t total_hint = hz3_large_total_cached_hint_load();
        size_t projected_hint = total_hint + hdr->map_size;
        if (projected_hint >= total_hint && projected_hint <= hard_cap) {
            can_cache = 1;
            hz3_large_aligned_obs_on_admit_precheck_yes();
#if HZ3_LARGE_CACHE_BUDGET
            do_madvise = (projected_hint > HZ3_LARGE_CACHE_SOFT_BYTES);
            if (do_madvise) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                madvise_start = (start + 4095) & ~(uintptr_t)4095;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (madvise_start < end) {
                    madvise_size = (end - madvise_start) & ~(size_t)4095;
                }
            }
#endif
        }
    }
#endif

    if (!can_cache) {
        hz3_large_cache_lock_acquire();

#if HZ3_S50_LARGE_SCACHE_EVICT
#if HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
        // Cap 超過時: evict してスペースを作る
        // S51: 同一 class を優先しつつ、victim をまとめて lock 外 munmap。
        for (;;) {
            Hz3LargeHdr* victims[HZ3_S185_EVICT_BATCH_MAX];
            size_t vcount = hz3_large_collect_evict_victims_locked(
                sc, hdr->map_size, hard_cap, victims, HZ3_S185_EVICT_BATCH_MAX, &evict_budget_left);
            if (vcount == 0) {
                break;
            }
            hz3_large_cache_lock_release();
            hz3_large_munmap_victims_unlocked(victims, vcount);
            hz3_large_cache_lock_acquire();
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#else
        // Cap 超過時: evict してスペースを作る
        // S51: 同一 class から先に evict（キャッシュ hit 率向上）
        while (hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
               evict_budget_left > 0) {
            Hz3LargeHdr* victim = NULL;
            // First: try same class
            victim = hz3_large_sc_pop_head(sc);
            if (!victim) {
                // Fallback: largest class
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    victim = hz3_large_sc_pop_head(i);
                    if (victim) {
                        break;
                    }
                }
            }
            if (!victim) break;  // evict 対象なし → munmap へ
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();  // hard 発火時のみ出力
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_large_cache_lock_release();
            hz3_large_dispose_victim_unlocked(victim);
            hz3_large_cache_lock_acquire();
            evict_budget_left--;
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#endif
#endif

        can_cache = (hz3_large_total_cached_load() + hdr->map_size <= insert_cap);
        hz3_large_aligned_obs_on_admit_lock_decision(can_cache);
#if HZ3_LARGE_CACHE_BUDGET
        if (can_cache && !do_madvise) {
            do_madvise = ((hz3_large_total_cached_load() + hdr->map_size) > HZ3_LARGE_CACHE_SOFT_BYTES);
            if (do_madvise) {
                uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
                madvise_start = (start + 4095) & ~(uintptr_t)4095;
                uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
                if (madvise_start < end) {
                    madvise_size = (end - madvise_start) & ~(size_t)4095;
                }
            }
        }
#endif

#if HZ3_S191_LARGE_FREE_FAST_INSERT && !HZ3_S207_TARGETED_REUSE
        // S191: If no madvise work is needed, keep the first lock and insert directly.
        if (can_cache) {
            int needs_unlock_for_madvise = 0;
#if HZ3_LARGE_CACHE_BUDGET
            needs_unlock_for_madvise = (do_madvise && madvise_size > 0);
#elif HZ3_S51_LARGE_MADVISE
            needs_unlock_for_madvise = 1;
#endif
            if (!needs_unlock_for_madvise) {
                if (hz3_large_total_cached_load() + hdr->map_size <= insert_cap &&
                    hz3_s293_global_admit_under_cap_locked(sc, hdr) &&
                    hz3_s293d_global_admit_under_soft_cap_locked(sc, hdr)) {
                    hdr->in_use = 0;
                    hdr->next = NULL;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
                    hz3_s276_direct_clear_retained(hdr);
#endif
                    hz3_large_s240_on_cache_store(hdr, sc);
                    hz3_s292_on_global_store(hdr);
                    hz3_large_sc_push_head(sc, hdr);
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
                    s270_victim_count =
                        hz3_s270_collect_idle_trim(sc,
                                                   s270_victims,
                                                   HZ3_S270_TRIM_BATCH_MAX);
#endif
                    hz3_large_stats_on_free_cached();
                    hz3_large_aligned_obs_on_admit_cache_insert();
                    hz3_large_cache_lock_release();
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
                    for (size_t i = 0; i < s270_victim_count; i++) {
                        hz3_s270_dispose_detached_victim(
                            s270_victims[i],
                            "s270-free-fast-insert-dispose");
                    }
#endif
                    hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
                    hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
                    hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
                    return 1;
                }
                hz3_large_aligned_obs_on_admit_final_fit_fail();
                can_cache = 0;
            }
        }
#endif

        hz3_large_cache_lock_release();
    }

    if (can_cache) {
        // S53: madvise は “cacheに入れる前” に実行する（reuse race 回避）
        // NOTE: hdr is not visible in any global list at this point.

        // S53: madvise は lock 外で実行
#if HZ3_LARGE_CACHE_BUDGET
#if HZ3_S193_DEMAND_SCAVENGE
        const int run_free_side_purge = 0;
#else
        const int run_free_side_purge = 1;
#endif
        if (run_free_side_purge && do_madvise && madvise_size > 0) {
#if HZ3_S53_THROTTLE
            // S53-2: Throttle check
            int do_actual_madvise = 1;
            uint32_t fc = atomic_fetch_add(&g_hz3_s53_free_counter, 1);
            if ((fc % HZ3_S53_MADVISE_MIN_INTERVAL) != 0) {
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_throttle_skips, 1);
#endif
                do_actual_madvise = 0;
                goto s53_done;
            }

            int64_t npages = (int64_t)(madvise_size >> 12);
            int64_t old_counter = atomic_fetch_sub(&g_hz3_s53_scavenge_counter_pages, npages);
            if (old_counter - npages >= 0) {
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_throttle_skips, 1);
#endif
                do_actual_madvise = 0;
                goto s53_done;
            }

            // 発火
            atomic_store(&g_hz3_s53_scavenge_counter_pages, HZ3_S53_MADVISE_RATE_PAGES);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_throttle_fires, 1);
#endif
s53_done:
            if (do_actual_madvise) {
                hz3_large_soft_purge((void*)madvise_start, madvise_size);
#if HZ3_LARGE_CACHE_STATS
                atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
                atomic_fetch_add(&g_budget_soft_hits, 1);
#endif
            }
#if HZ3_LARGE_CACHE_STATS
            hz3_large_cache_stats_dump();
#endif
#else
            // THROTTLE=0: 従来通り毎回 madvise
            hz3_large_soft_purge((void*)madvise_start, madvise_size);
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_madvise_bytes, madvise_size);
            atomic_fetch_add(&g_budget_soft_hits, 1);
            hz3_large_cache_stats_dump();
#endif
#endif
        }
#elif HZ3_S51_LARGE_MADVISE
#if !HZ3_S193_DEMAND_SCAVENGE
        // S51: 既存の常時 madvise（BUDGET 無効時のみ）
        {
            uintptr_t start = (uintptr_t)hdr->map_base + hz3_large_user_offset();
            uintptr_t aligned_start = (start + 4095) & ~(uintptr_t)4095;  // next page
            uintptr_t end = (uintptr_t)hdr->map_base + hdr->map_size;
            if (aligned_start < end) {
                size_t aligned_size = (end - aligned_start) & ~(size_t)4095;  // page truncate
                if (aligned_size > 0) {
                    hz3_large_soft_purge((void*)aligned_start, aligned_size);
                }
            }
        }
#endif
#endif
        // Finally: insert into cache (under lock). If we can no longer fit,
        // fall back to munmap (safe, but may lose caching benefits).
        hz3_large_cache_lock_acquire();

        // Recompute hard cap under lock (another thread may have changed totals)
        hard_cap = hz3_large_hard_cap_for_sc(sc);
        insert_cap = hz3_large_insert_cap_for_sc(sc, hard_cap);

#if HZ3_S50_LARGE_SCACHE_EVICT
#if HZ3_S185_LARGE_EVICT_MUNMAP_BATCH
        for (;;) {
            Hz3LargeHdr* victims[HZ3_S185_EVICT_BATCH_MAX];
            size_t vcount = hz3_large_collect_evict_victims_locked(
                sc, hdr->map_size, hard_cap, victims, HZ3_S185_EVICT_BATCH_MAX, &evict_budget_left);
            if (vcount == 0) {
                break;
            }
            hz3_large_cache_lock_release();
            hz3_large_munmap_victims_unlocked(victims, vcount);
            hz3_large_cache_lock_acquire();
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#else
        while (hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
               evict_budget_left > 0) {
            Hz3LargeHdr* victim = NULL;
            victim = hz3_large_sc_pop_head(sc);
            if (!victim) {
                for (int i = HZ3_LARGE_SC_COUNT - 2; i >= 0; i--) {
                    victim = hz3_large_sc_pop_head(i);
                    if (victim) {
                        break;
                    }
                }
            }
            if (!victim) break;
#if HZ3_LARGE_CACHE_STATS
            atomic_fetch_add(&g_budget_hard_evicts, 1);
            hz3_large_cache_stats_dump();
#endif
            hz3_large_debug_on_munmap_locked(victim);
            hz3_large_cache_lock_release();
            hz3_large_dispose_victim_unlocked(victim);
            hz3_large_cache_lock_acquire();
            evict_budget_left--;
        }
        if (!evict_budget_exhausted &&
            hz3_large_total_cached_load() + hdr->map_size > hard_cap &&
            evict_budget_left == 0) {
            evict_budget_exhausted = 1;
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            hz3_large_stat_inc(&g_s237_budget_exhausted);
#endif
        }
#endif
#endif

        if (hz3_large_total_cached_load() + hdr->map_size <= insert_cap &&
            hz3_s293_global_admit_under_cap_locked(sc, hdr) &&
            hz3_s293d_global_admit_under_soft_cap_locked(sc, hdr)) {
#if HZ3_LARGE_CACHE_STATS && HZ3_S237_HIBAND_RETAIN_TUNE
            if (hz3_large_total_cached_load() + hdr->map_size > hard_cap) {
                hz3_large_stat_inc(&g_s237_slack_admit_hits);
            }
#endif
            if (!hz3_large_reuse_try_admit_locked(sc)) {
                hz3_large_aligned_obs_on_admit_reuse_skip();
                hz3_large_cache_lock_release();
                goto do_munmap;
            }
            hdr->in_use = 0;
            hdr->next = NULL;
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
            hz3_s276_direct_clear_retained(hdr);
#endif
            hz3_large_s240_on_cache_store(hdr, sc);
            hz3_s292_on_global_store(hdr);
            hz3_large_sc_push_head(sc, hdr);
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
            s270_victim_count =
                hz3_s270_collect_idle_trim(sc, s270_victims, HZ3_S270_TRIM_BATCH_MAX);
#endif
            hz3_large_stats_on_free_cached();
            hz3_large_aligned_obs_on_admit_cache_insert();
            hz3_large_cache_lock_release();
#if HZ3_S270_LARGE_SCACHE_IDLE_TRIM
            for (size_t i = 0; i < s270_victim_count; i++) {
                hz3_s270_dispose_detached_victim(s270_victims[i],
                                                 "s270-free-final-insert-dispose");
            }
#endif
            hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
            hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
            hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
            return 1;
        }

        hz3_large_cache_lock_release();
        hz3_large_aligned_obs_on_admit_final_fit_fail();
        goto do_munmap;
    }

    // Cache できない（cap超過など）: munmap へ
#else
    // Legacy: 上限チェック
    hz3_large_cache_lock_acquire();
    if (g_hz3_large_cached_bytes + hdr->map_size <= HZ3_LARGE_CACHE_MAX_BYTES &&
        g_hz3_large_cached_nodes < HZ3_LARGE_CACHE_MAX_NODES) {
        // Cache に追加
        hdr->in_use = 0;
        hdr->next = NULL;           // bucket list から切断済み
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
        hz3_s276_direct_clear_retained(hdr);
#endif
        hz3_large_s240_on_cache_store(hdr, hz3_large_sc(hdr->map_size));
        hz3_s292_on_global_store(hdr);
        hdr->next_free = g_hz3_large_free_head;
        g_hz3_large_free_head = hdr;
        g_hz3_large_cached_bytes += hdr->map_size;
        g_hz3_large_cached_nodes++;
        hz3_large_stats_on_free_cached();
        hz3_large_aligned_obs_on_admit_cache_insert();
        hz3_large_cache_lock_release();
        hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
        hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
        hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
        return 1;
    }
    hz3_large_cache_lock_release();
#endif
#endif

#if HZ3_S50_LARGE_SCACHE && HZ3_LARGE_CACHE_ENABLE
do_munmap:
#endif
    if (!s243_free_fallback_active) {
        s243_free_fallback_start_ns = hz3_s243_residual_timing_start();
        s243_free_fallback_active = 1;
    }
    // Cache full または無効: munmap
    hz3_large_aligned_obs_on_admit_do_munmap();
    hz3_s280_free_route_on_munmap();
    hz3_large_stats_on_free_munmap();
#if HZ3_S276_LARGE_DIRECT_RETAIN_FRONT || HZ3_S276_LARGE_DIRECT_RETAIN_INBOX
    hz3_s276_direct_clear_retained(hdr);
#endif
    hz3_large_s240_on_munmap_store(hdr);
    hz3_s292_on_global_munmap(hdr);
    hdr->magic = 0;
#if HZ3_LARGE_CACHE_ENABLE && HZ3_S50_LARGE_SCACHE && HZ3_S212_LARGE_UNMAP_DEFER_PLUS
    if (hz3_large_try_defer_direct_unmap(hdr)) {
        hz3_large_aligned_obs_on_admit_defer_munmap();
        hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
        hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
        hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
        return 1;
    }
#endif
    hz3_large_cache_lock_acquire();
    hz3_large_debug_on_munmap_locked(hdr);
    hz3_large_cache_lock_release();
    hz3_large_os_munmap(hdr->map_base, hdr->map_size);
    hz3_large_aligned_obs_on_free_admit_elapsed(obs_free_admit_start_ns);
    hz3_s243_residual_on_free_fallback_global_elapsed(s243_free_fallback_start_ns);
    hz3_s243_residual_on_free_total_elapsed(s243_free_total_start_ns);
    return 1;
}
