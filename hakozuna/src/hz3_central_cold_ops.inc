#if HZ3_S65_CENTRAL_COLD_ENABLE
void hz3_central_cold_push_list(int shard, int sc, void* head, void* tail, uint32_t n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return;
    if (sc < 0 || sc >= HZ3_NUM_SC) return;
    if (!head || !tail || n == 0) return;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    Hz3CentralColdExternalBin* bin = &g_hz3_central_cold_ext[shard][sc];
    uint32_t pushed = 0;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    uint32_t ready_pushed = 0;
    uint32_t ready_popped = 0;
    uint32_t decommitted_pushed = 0;
#endif
    void* cur = head;
    HZ3_COLD_DECOMMIT_OBS_DECL(decommit_obs);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
    Hz3CentralColdNode* decommit_nodes[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS];
    uint8_t decommit_ok[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS];
#endif

    hz3_lock_acquire(&bin->lock);
    while (pushed < n && cur) {
        void* next = hz3_obj_get_next(cur);
        Hz3CentralColdNode* node = hz3_central_cold_external_node_alloc(cur);
        if (!node) {
            break;
        }
        hz3_obj_set_next(cur, NULL);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
        hz3_central_cold_ready_push_head_locked(bin, node);
        ready_pushed++;
        bin->count++;
        pushed++;

#if !HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
        uint32_t target = hz3_central_cold_ready_target_runs(sc);
        while (bin->ready_count > target) {
            Hz3CentralColdNode* evict = hz3_central_cold_ready_pop_tail_locked(bin);
            if (!evict) {
                break;
            }
            ready_popped++;
            evict->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTING;
            bin->inflight_decommitting++;
            hz3_s65_cold_note_inflight_decommitting_add(1);
            HZ3_COLD_DECOMMIT_OBS_NOTE(decommit_obs, evict, sc);

            hz3_lock_release(&bin->lock);
            int decommit_ok =
                hz3_central_cold_decommit_run(evict->run, hz3_central_cold_run_bytes(sc)) == 0;
            hz3_lock_acquire(&bin->lock);

            if (bin->inflight_decommitting > 0) {
                bin->inflight_decommitting--;
            }
            hz3_s65_cold_note_inflight_decommitting_sub(1);
            if (decommit_ok) {
                hz3_central_cold_decommitted_push_head_locked(bin, evict);
                decommitted_pushed++;
            } else {
                hz3_central_cold_ready_push_head_locked(bin, evict);
                ready_pushed++;
            }
        }
#else
        if (!hz3_central_cold_decommit_coalesce_enabled_for_sc(sc)) {
            uint32_t target = hz3_central_cold_ready_target_runs(sc);
            while (bin->ready_count > target) {
                Hz3CentralColdNode* evict = hz3_central_cold_ready_pop_tail_locked(bin);
                if (!evict) {
                    break;
                }
                ready_popped++;
                evict->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTING;
                bin->inflight_decommitting++;
                hz3_s65_cold_note_inflight_decommitting_add(1);
                HZ3_COLD_DECOMMIT_OBS_NOTE(decommit_obs, evict, sc);

                hz3_lock_release(&bin->lock);
                int decommit_ok =
                    hz3_central_cold_decommit_run(evict->run, hz3_central_cold_run_bytes(sc)) == 0;
                hz3_lock_acquire(&bin->lock);

                if (bin->inflight_decommitting > 0) {
                    bin->inflight_decommitting--;
                }
                hz3_s65_cold_note_inflight_decommitting_sub(1);
                if (decommit_ok) {
                    hz3_central_cold_decommitted_push_head_locked(bin, evict);
                    decommitted_pushed++;
                } else {
                    hz3_central_cold_ready_push_head_locked(bin, evict);
                    ready_pushed++;
                }
            }
        }
#endif
#else
        int did_decommit = 0;
        if (hz3_central_cold_should_decommit_locked(bin, sc)) {
            if (hz3_central_cold_decommit_run(cur, hz3_central_cold_run_bytes(sc)) == 0) {
                node->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTED;
                did_decommit = 1;
            }
        }
        if (did_decommit) {
            HZ3_COLD_DECOMMIT_OBS_NOTE(decommit_obs, node, sc);
            hz3_central_cold_decommitted_push_head_locked(bin, node);
            decommitted_pushed++;
        } else {
            hz3_central_cold_ready_push_head_locked(bin, node);
            ready_pushed++;
        }
        bin->count++;
        pushed++;
#endif
#else
        node->prev = NULL;
        node->next = bin->ready_head;
        if (bin->ready_head) {
            bin->ready_head->prev = node;
        } else {
            bin->ready_tail = node;
        }
        bin->ready_head = node;
        bin->count++;
        pushed++;
#endif
        cur = next;
    }
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
    if (hz3_central_cold_decommit_coalesce_enabled_for_sc(sc)) {
        uint32_t target = hz3_central_cold_ready_target_runs(sc);
        int stop_after_failure = 0;
        while (bin->ready_count > target && !stop_after_failure) {
            uint32_t detached = 0;
            while (detached < HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS &&
                   bin->ready_count > target) {
                Hz3CentralColdNode* evict = hz3_central_cold_ready_pop_tail_locked(bin);
                if (!evict) {
                    break;
                }
                ready_popped++;
                evict->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTING;
                decommit_nodes[detached++] = evict;
            }
            if (detached == 0) {
                break;
            }

            bin->inflight_decommitting += detached;
            hz3_s65_cold_note_inflight_decommitting_add(detached);

            hz3_lock_release(&bin->lock);
            hz3_central_cold_decommit_nodes_coalesced(decommit_nodes,
                                                      decommit_ok,
                                                      detached,
                                                      sc);
            hz3_lock_acquire(&bin->lock);

            if (bin->inflight_decommitting >= detached) {
                bin->inflight_decommitting -= detached;
            } else {
                bin->inflight_decommitting = 0;
            }
            hz3_s65_cold_note_inflight_decommitting_sub(detached);

            uint32_t failed = 0;
            for (uint32_t i = 0; i < detached; i++) {
                if (decommit_ok[i]) {
                    hz3_central_cold_decommitted_push_head_locked(bin, decommit_nodes[i]);
                    decommitted_pushed++;
                } else {
                    hz3_central_cold_ready_push_head_locked(bin, decommit_nodes[i]);
                    ready_pushed++;
                    failed++;
                }
            }
            if (failed > 0) {
                stop_after_failure = 1;
            }
        }
    }
#endif
    hz3_lock_release(&bin->lock);
    HZ3_COLD_DECOMMIT_OBS_FLUSH(decommit_obs, sc);

    if (pushed > 0) {
        hz3_s65_cold_note_push(sc, pushed);
    }
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    hz3_s65_cold_note_committed_push(sc, ready_pushed);
    hz3_s65_cold_note_committed_pop(sc, ready_popped);
    hz3_s65_cold_note_decommitted_push(sc, decommitted_pushed);
#endif

    if (pushed < n && cur) {
        uint32_t fallback_n = n - pushed;
        hz3_s65_cold_ext_note_fallback_hot(fallback_n);
        hz3_central_push_list(shard, sc, cur, tail, fallback_n);
    }
#else
    Hz3CentralBin* bin = &g_hz3_central_cold[shard][sc];
    hz3_lock_acquire(&bin->lock);
    hz3_obj_set_next(tail, bin->head);
    bin->head = head;
    bin->count += n;
    hz3_s65_cold_note_push(sc, n);
    hz3_lock_release(&bin->lock);
#endif
}

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static int hz3_central_cold_pop_batch_mru_ready(Hz3CentralColdExternalBin* bin,
                                                int sc,
                                                void** out,
                                                int want) {
    hz3_lock_acquire(&bin->lock);

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
    if (!bin->ready_head &&
        bin->decommitted_head &&
        hz3_sc_to_pages(sc) >= HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES) {
        hz3_lock_release(&bin->lock);
        (void)hz3_central_cold_rewarm_decommitted_outside_lock(
            bin,
            sc,
            (uint32_t)HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS);
        hz3_lock_acquire(&bin->lock);
    }
#endif
#if HZ3_S65_CENTRAL_COLD_SMALL_REWARM_ON_MISS
    int small_rewarm_pages = hz3_sc_to_pages(sc);
    if (!bin->ready_head &&
        bin->decommitted_head &&
        small_rewarm_pages >= HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MIN_PAGES &&
        small_rewarm_pages <= HZ3_S65_CENTRAL_COLD_SMALL_REWARM_MAX_PAGES) {
        hz3_lock_release(&bin->lock);
        (void)hz3_central_cold_rewarm_decommitted_outside_lock(
            bin,
            sc,
            (uint32_t)HZ3_S65_CENTRAL_COLD_SMALL_REWARM_BATCH_RUNS);
        hz3_lock_acquire(&bin->lock);
    }
#endif

    int got = 0;
    while (got < want && bin->ready_head) {
        Hz3CentralColdNode* node = hz3_central_cold_ready_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (bin->count > 0) {
            bin->count--;
        }
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }

    hz3_s65_cold_note_ready_pop_attempt((uint64_t)got);
    hz3_s65_cold_note_pop(sc, (uint64_t)got);
    hz3_s65_cold_note_committed_pop(sc, (uint64_t)got);
    hz3_lock_release(&bin->lock);
    return got;
}
#endif

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE && \
    HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && \
    HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK
static int hz3_central_cold_pop_batch_unlocked_recommit(Hz3CentralColdExternalBin* bin,
                                                        int sc,
                                                        void** out,
                                                        int want) {
    if (want > 16) {
        want = 16;
    }

    Hz3CentralColdNode* committed_nodes[16];
    Hz3CentralColdNode* decommitted_nodes[16];
    int committed_got = 0;
    int decommitted_got = 0;

    hz3_lock_acquire(&bin->lock);
    while (committed_got < want && bin->ready_head) {
        Hz3CentralColdNode* node = hz3_central_cold_ready_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (bin->count > 0) {
            bin->count--;
        }
        committed_nodes[committed_got++] = node;
    }

    while ((committed_got + decommitted_got) < want && bin->decommitted_head) {
        Hz3CentralColdNode* node = hz3_central_cold_decommitted_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (bin->count > 0) {
            bin->count--;
        }
        node->state = HZ3_CENTRAL_COLD_STATE_RECOMMITTING;
        bin->inflight_recommitting++;
        hz3_s65_cold_note_inflight_recommitting_add(1);
        decommitted_nodes[decommitted_got++] = node;
    }
    hz3_lock_release(&bin->lock);

    int got = 0;
    for (int i = 0; i < committed_got; i++) {
        Hz3CentralColdNode* node = committed_nodes[i];
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }

    for (int i = 0; i < decommitted_got; i++) {
        Hz3CentralColdNode* node = decommitted_nodes[i];
        if (node->state == HZ3_CENTRAL_COLD_STATE_RECOMMITTING &&
            hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) != 0) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] unlocked recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }

    if (decommitted_got > 0) {
        hz3_lock_acquire(&bin->lock);
        if (bin->inflight_recommitting >= (uint32_t)decommitted_got) {
            bin->inflight_recommitting -= (uint32_t)decommitted_got;
        } else {
            bin->inflight_recommitting = 0;
        }
        hz3_lock_release(&bin->lock);
        hz3_s65_cold_note_inflight_recommitting_sub((uint64_t)decommitted_got);
    }

    hz3_s65_cold_note_pop(sc, (uint64_t)got);
    hz3_s65_cold_note_committed_pop(sc, (uint64_t)committed_got);
    hz3_s65_cold_note_decommitted_pop(sc, (uint64_t)decommitted_got);
    hz3_s65_cold_note_unlock_recommit((uint64_t)decommitted_got);
    return got;
}
#endif

int hz3_central_cold_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;
    if (!out || want <= 0) return 0;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    Hz3CentralColdExternalBin* bin = &g_hz3_central_cold_ext[shard][sc];
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
    return hz3_central_cold_pop_batch_mru_ready(bin, sc, out, want);
#endif
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE && HZ3_S65_CENTRAL_COLD_RECOMMIT_OUTSIDE_LOCK
    return hz3_central_cold_pop_batch_unlocked_recommit(bin, sc, out, want);
#endif
    hz3_lock_acquire(&bin->lock);
    int got = 0;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    int committed_got = 0;
    int decommitted_got = 0;
#endif
    while (got < want && (bin->ready_head
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
                          || bin->decommitted_head
#endif
                          )) {
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
        if (!bin->ready_head &&
            bin->decommitted_head &&
            hz3_sc_to_pages(sc) >= HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_MIN_PAGES) {
            (void)hz3_central_cold_rewarm_decommitted_locked(
                bin,
                sc,
                (uint32_t)HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS);
        }
#endif
        Hz3CentralColdNode* node = NULL;
        if (bin->ready_head) {
            node = hz3_central_cold_ready_pop_head_locked(bin);
            if (!node) {
                break;
            }
            committed_got++;
        } else {
            node = hz3_central_cold_decommitted_pop_head_locked(bin);
            if (!node) {
                break;
            }
            decommitted_got++;
        }
#else
        Hz3CentralColdNode* node = bin->ready_head;
        bin->ready_head = node->next;
        if (bin->ready_head) {
            bin->ready_head->prev = NULL;
        } else {
            bin->ready_tail = NULL;
        }
        node->next = NULL;
        node->prev = NULL;
#endif
        bin->count--;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
        if (hz3_central_cold_node_is_decommitted(node) &&
            hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) != 0) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
#endif
        out[got++] = node->run;
        hz3_central_cold_external_node_free(node);
    }
    hz3_s65_cold_note_pop(sc, (uint64_t)got);
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    hz3_s65_cold_note_committed_pop(sc, (uint64_t)committed_got);
    hz3_s65_cold_note_decommitted_pop(sc, (uint64_t)decommitted_got);
#endif
    hz3_lock_release(&bin->lock);
    return got;
#else
    Hz3CentralBin* bin = &g_hz3_central_cold[shard][sc];
    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* cur = bin->head;
        void* next = hz3_obj_get_next(cur);
        out[got++] = cur;
        bin->head = next;
        bin->count--;
    }
    hz3_s65_cold_note_pop(sc, (uint64_t)got);
    hz3_lock_release(&bin->lock);
    return got;
#endif
}

uint32_t hz3_central_cold_count_snapshot(int shard, int sc) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (sc < 0 || sc >= HZ3_NUM_SC) return 0;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
    Hz3CentralColdExternalBin* bin = &g_hz3_central_cold_ext[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
#else
    Hz3CentralBin* bin = &g_hz3_central_cold[shard][sc];
    hz3_lock_acquire(&bin->lock);
    uint32_t count = bin->count;
    hz3_lock_release(&bin->lock);
    return count;
#endif
}
#endif
