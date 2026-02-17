// hz4_tcbox.c - TransferCacheBox (Phase 20B) グローバル変数
#include "hz4_tcbox.h"

#if HZ4_TRANSFERCACHE
// Global transfer cache (zero-initialized)
hz4_tcbox_stack_t g_hz4_tcbox[HZ4_TRANSFERCACHE_SHARDS][HZ4_SC_MAX];
#endif
