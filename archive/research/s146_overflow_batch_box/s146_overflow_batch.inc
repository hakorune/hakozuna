// S146-B OverflowBatchBox (archived snippet)
// NOTE: This is archived and NOT used in mainline.

#if HZ3_S146_OVERFLOW_BATCH
static inline void hz3_s146_overflow_flush(Hz3TCache* tc, int sc) {
    uint8_t cnt = tc->s146_overflow_count[sc];
    if (cnt == 0) return;

    uint8_t owner = tc->s146_overflow_owner[sc];
    void* head = tc->s146_overflow_buf[sc][0];
    void* tail = head;
    for (uint8_t i = 1; i < cnt; i++) {
        hz3_obj_set_next(tail, tc->s146_overflow_buf[sc][i]);
        tail = tc->s146_overflow_buf[sc][i];
    }
    hz3_obj_set_next(tail, NULL);

    if (!hz3_owner_stash_push_list(owner, sc, head, tail, cnt)) {
        hz3_small_xfer_push_list(owner, sc, head, tail, cnt);
    }
    tc->s146_overflow_count[sc] = 0;
}
#endif

#if HZ3_S111_REMOTE_PUSH_N1
    if (n == 1) {
#if HZ3_S146_OVERFLOW_BATCH
        Hz3TCache* tc = &t_hz3_cache;
        uint8_t cnt = tc->s146_overflow_count[sc];
        uint8_t buf_owner = tc->s146_overflow_owner[sc];

        if (cnt > 0 && buf_owner != owner) {
            hz3_s146_overflow_flush(tc, sc);
            cnt = 0;
        }

        tc->s146_overflow_buf[sc][cnt] = head;
        tc->s146_overflow_count[sc] = cnt + 1;
        tc->s146_overflow_owner[sc] = owner;

        if (cnt + 1 >= HZ3_S146_OVERFLOW_BATCH_N) {
            hz3_s146_overflow_flush(tc, sc);
        }
        return;
#else
        if (hz3_owner_stash_push_one(owner, sc, head)) {
            return;
        }
#endif
    }
#endif
