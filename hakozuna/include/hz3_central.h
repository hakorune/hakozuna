#pragma once

#include "hz3_types.h"
#include <pthread.h>

// ============================================================================
// Central pool: per-shard, per-sc run storage
// Day 3: simple mutex-based implementation (correctness first)
// ============================================================================

typedef struct {
    pthread_mutex_t lock;
    void* head;           // run list (intrusive, next ptr at run[0])
    uint32_t count;
} Hz3CentralBin;

// Global central pool
extern Hz3CentralBin g_hz3_central[HZ3_NUM_SHARDS][HZ3_NUM_SC];

// ============================================================================
// Central API
// ============================================================================

// Push a run to central (thread-safe)
void hz3_central_push(int shard, int sc, void* run);

// Pop a run from central (thread-safe, returns NULL if empty)
void* hz3_central_pop(int shard, int sc);

// Initialize central pool (thread-safe via pthread_once)
void hz3_central_init(void);

// ============================================================================
// Day 5: Batch API
// ============================================================================

// Push a linked list to central (head→...→tail in forward order)
// tail->next will be set to old head
void hz3_central_push_list(int shard, int sc, void* head, void* tail, uint32_t n);

// Pop up to 'want' objects into out array (returns actual count)
int hz3_central_pop_batch(int shard, int sc, void** out, int want);
