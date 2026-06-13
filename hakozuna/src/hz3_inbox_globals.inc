// Global inbox pool (zero-initialized)
Hz3Inbox g_hz3_inbox[HZ3_NUM_SHARDS][HZ3_NUM_SC];
Hz3InboxMixed g_hz3_inbox_medium_mixed[HZ3_NUM_SHARDS];
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
Hz3Inbox g_hz3_inbox_medium_aligned[HZ3_NUM_SHARDS][HZ3_NUM_SC];
#endif

#if HZ3_S65_INBOX_TO_CENTRAL_RECLAIM && HZ3_S65_MEDIUM_RECLAIM && \
    HZ3_S65_INBOX_TO_CENTRAL_RECLAIM_BATCH_RUNS > 1
static _Atomic(uint64_t) g_s65_inbox_to_central_reclaim_pending[HZ3_NUM_SHARDS][HZ3_NUM_SC];
static _Atomic(uint64_t) g_s65_inbox_to_central_reclaim_batch_flushes;
static _Atomic(uint64_t) g_s65_inbox_to_central_reclaim_batch_budget;
static _Atomic(uint64_t) g_s65_inbox_to_central_reclaim_batch_pending_peak;
#endif
