#if HZ3_S189_MEDIUM_TRANSFERCACHE
int hz3_central_xfer_pop_batch(int shard, int sc, void** out, int want) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (!hz3_s189_sc_ok(sc)) return 0;
    if (!out || want <= 0) return 0;

    atomic_fetch_add_explicit(&g_s189_xfer_pop_calls, 1, memory_order_relaxed);
    Hz3CentralXferBin* bin = &g_hz3_central_xfer[shard][sc];

    hz3_lock_acquire(&bin->lock);
    int got = 0;
    while (got < want && bin->head) {
        void* run = bin->head;
        bin->head = hz3_obj_get_next(run);
        bin->count--;
        out[got++] = run;
    }
    hz3_lock_release(&bin->lock);

    if (got > 0) {
        atomic_fetch_add_explicit(&g_s189_xfer_pop_hits, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&g_s189_xfer_pop_objs, (uint64_t)got, memory_order_relaxed);
        hz3_s65_xfer_note_pop(sc, (uint64_t)got);
    }
    return got;
}

int hz3_central_xfer_push_array(int shard, int sc, void** arr, int n) {
    if (shard < 0 || shard >= HZ3_NUM_SHARDS) return 0;
    if (!hz3_s189_sc_ok(sc)) return 0;
    if (!arr || n <= 0) return 0;

    atomic_fetch_add_explicit(&g_s189_xfer_push_calls, 1, memory_order_relaxed);
    Hz3CentralXferBin* bin = &g_hz3_central_xfer[shard][sc];

    hz3_lock_acquire(&bin->lock);
    int avail = (int)HZ3_S189_MAX_OBJS_PER_BIN - (int)bin->count;
    if (avail < 0) avail = 0;
    int keep = (n < avail) ? n : avail;
    int pushed = 0;
    for (int i = 0; i < keep; i++) {
        void* run = arr[i];
        if (!run) continue;
        hz3_obj_set_next(run, bin->head);
        bin->head = run;
        bin->count++;
        pushed++;
    }
    hz3_lock_release(&bin->lock);

    if (pushed > 0) {
        atomic_fetch_add_explicit(&g_s189_xfer_push_objs, (uint64_t)pushed, memory_order_relaxed);
        hz3_s65_xfer_note_push(sc, (uint64_t)pushed);
    }
    if (n > pushed) {
        atomic_fetch_add_explicit(&g_s189_xfer_push_drop, (uint64_t)(n - pushed), memory_order_relaxed);
    }
    return pushed;
}
#endif
