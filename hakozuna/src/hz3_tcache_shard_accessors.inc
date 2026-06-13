int hz3_shard_collision_detected(void) {
    return atomic_load_explicit(&g_shard_collision_detected, memory_order_acquire) != 0;
}

uint32_t hz3_shard_live_count(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) {
        return 0;
    }
    return atomic_load_explicit(&g_shard_live_count[shard], memory_order_acquire);
}

int hz3_shard_is_exiting(uint8_t shard) {
    if (shard >= HZ3_NUM_SHARDS) {
        return 0;
    }
    return atomic_load_explicit(&g_shard_exiting[shard], memory_order_acquire) != 0;
}
