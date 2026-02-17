// hz4_tcbox.h - TransferCacheBox データ構造（Phase 20B）
#ifndef HZ4_TCBOX_H
#define HZ4_TCBOX_H

#include "hz4_types.h"
#include "hz4_config.h"

#if HZ4_TRANSFERCACHE

// Batch node（intrusive list）
typedef struct hz4_tcbox_batch {
    void* head;
    void* tail;
    uint16_t count;
    uint8_t sc;
    uint8_t _pad;
    struct hz4_tcbox_batch* next;
} hz4_tcbox_batch_t;

// Per-shard Treiber stack (per owner, per sc)
typedef struct {
    _Atomic(hz4_tcbox_batch_t*) head;
} hz4_tcbox_stack_t;

// Global transfer cache (HZ4_TRANSFERCACHE_SHARDS x HZ4_SC_MAX)
extern hz4_tcbox_stack_t g_hz4_tcbox[HZ4_TRANSFERCACHE_SHARDS][HZ4_SC_MAX];

#endif // HZ4_TRANSFERCACHE

#endif // HZ4_TCBOX_H
