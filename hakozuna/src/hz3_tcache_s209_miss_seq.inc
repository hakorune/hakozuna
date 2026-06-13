#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG || HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
static _Atomic uint32_t g_s209_medium_miss_seq[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#endif

uint32_t hz3_s209_medium_miss_seq_load(uint8_t shard, int sc) {
#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG || HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    if ((uint32_t)shard >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return 0;
    }
    return atomic_load_explicit(&g_s209_medium_miss_seq[shard][sc], memory_order_relaxed);
#else
    (void)shard;
    (void)sc;
    return 0;
#endif
}

void hz3_s209_medium_miss_seq_note(uint8_t shard, int sc, uint32_t miss_count) {
#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG || HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    if ((uint32_t)shard >= (uint32_t)HZ3_NUM_SHARDS ||
        (uint32_t)sc >= (uint32_t)HZ3_NUM_SC) {
        return;
    }
    if (miss_count == 0) {
        return;
    }

#if HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG && HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    int in_s209_range = (sc >= HZ3_S209_SC_MIN && sc <= HZ3_S209_SC_MAX);
    int in_s210_range = (sc >= HZ3_S210_SC_MIN && sc <= HZ3_S210_SC_MAX);
    if (!in_s209_range && !in_s210_range) {
        return;
    }
    int stride_hit = 0;
    if (in_s209_range &&
        (miss_count % (uint32_t)HZ3_S209_MISS_STRIDE) == 0u) {
        stride_hit = 1;
    }
    if (!stride_hit && in_s210_range &&
        (miss_count % (uint32_t)HZ3_S210_MISS_STRIDE) == 0u) {
        stride_hit = 1;
    }
    if (!stride_hit) {
        return;
    }
#elif HZ3_S209_MEDIUM_OWNER_RESCUE_STRONG
    if (sc < HZ3_S209_SC_MIN || sc > HZ3_S209_SC_MAX) {
        return;
    }
    if ((miss_count % (uint32_t)HZ3_S209_MISS_STRIDE) != 0u) {
        return;
    }
#elif HZ3_S210_MEDIUM_OWNER_RESCUE_LITE
    if (sc < HZ3_S210_SC_MIN || sc > HZ3_S210_SC_MAX) {
        return;
    }
    if ((miss_count % (uint32_t)HZ3_S210_MISS_STRIDE) != 0u) {
        return;
    }
#endif

    atomic_fetch_add_explicit(&g_s209_medium_miss_seq[shard][sc], 1, memory_order_relaxed);
#else
    (void)shard;
    (void)sc;
    (void)miss_count;
#endif
}
