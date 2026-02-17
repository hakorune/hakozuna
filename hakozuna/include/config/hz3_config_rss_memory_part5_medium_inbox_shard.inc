// S187: MediumPathCounterTriageBox (opt-in counters)
#ifndef HZ3_MEDIUM_PATH_STATS
#define HZ3_MEDIUM_PATH_STATS 0
#endif

// S195: MediumInboxShardBox (opt-in)
// Split medium inbox head/count into N sub-shards to reduce MPSC CAS contention
// on remote-heavy medium free paths. 1 = legacy behavior.
#ifndef HZ3_S195_MEDIUM_INBOX_SHARDS
#define HZ3_S195_MEDIUM_INBOX_SHARDS 1
#endif

// Debug-only counters for inbox push/drain contention (atexit one-shot).
#ifndef HZ3_S195_MEDIUM_INBOX_STATS
#define HZ3_S195_MEDIUM_INBOX_STATS 0
#endif

#if HZ3_S195_MEDIUM_INBOX_SHARDS < 1
#error "HZ3_S195_MEDIUM_INBOX_SHARDS must be >= 1"
#endif

#if (HZ3_S195_MEDIUM_INBOX_SHARDS & (HZ3_S195_MEDIUM_INBOX_SHARDS - 1)) != 0
#error "HZ3_S195_MEDIUM_INBOX_SHARDS must be power-of-2"
#endif

