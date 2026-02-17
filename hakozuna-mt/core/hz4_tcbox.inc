// hz4_tcbox.inc - TransferCacheBox 実装（Phase 20B）
#ifndef HZ4_TCBOX_INC
#define HZ4_TCBOX_INC

#include "hz4_tcbox.h"
#include "hz4_tls.h"
#include "hz4_page.h"
#include <stdatomic.h>
#include <stdlib.h>

#if HZ4_TRANSFERCACHE

// MIN macro for compile-time optimization
#define TCBOX_MIN(a, b) ((a) < (b) ? (a) : (b))

// Note: hz4_tcache_splice and hz4_tcache_bin_t are defined in hz4_tcache.c
// before this file is included.

static inline bool hz4_tcbox_trim_enabled(hz4_tls_t* tls) {
#if HZ4_TRANSFERCACHE_GATEBOX
#if HZ4_REMOTE_PAGE_RBUF_GATEBOX && !HZ4_TRANSFERCACHE_GATE_ALLOW_WHEN_RBUF_OFF
    if (tls->rbuf_gate_on == 0) {
        return false;
    }
#endif
#endif
    (void)tls;
    return true;
}

static inline bool hz4_tcbox_pop_enabled(hz4_tls_t* tls) {
#if HZ4_TRANSFERCACHE_GATEBOX && HZ4_TRANSFERCACHE_GATE_POP
    return hz4_tcbox_trim_enabled(tls);
#else
    (void)tls;
    return true;
#endif
}

// ============================================================================
// Batch node 確保/解放（TLS キャッシュ付き）
// ============================================================================

static inline hz4_tcbox_batch_t* hz4_tcbox_batch_alloc(hz4_tls_t* tls) {
    if (tls->tcbox_node_count > 0) {
        return tls->tcbox_node_cache[--tls->tcbox_node_count];
    }
    // Fallback to malloc (acceptable for cold path)
    return (hz4_tcbox_batch_t*)malloc(sizeof(hz4_tcbox_batch_t));
}

static inline void hz4_tcbox_batch_free(hz4_tls_t* tls, hz4_tcbox_batch_t* batch) {
    if (tls->tcbox_node_count < HZ4_TRANSFERCACHE_NODE_CACHE) {
        tls->tcbox_node_cache[tls->tcbox_node_count++] = batch;
    } else {
        free(batch);
    }
}

// ============================================================================
// Trim（bin → tcbox push）
// ============================================================================

// Collect 後に bin が大きすぎる場合に trim
// HZ4_TCACHE_COUNT=0 の時は呼び出し側でガードすること
static inline void hz4_tcbox_trim(hz4_tls_t* tls, uint8_t sc, hz4_tcache_bin_t* bin) {
#if !HZ4_TCACHE_COUNT
    // count がない場合は trim できない（何もしない）
    (void)tls; (void)sc; (void)bin;
    return;
#endif
    if (!hz4_tcbox_trim_enabled(tls)) {
        return;
    }

    // trim 閾値: TRANSFERCACHE_BATCH * 4 (例: 32 * 4 = 128)
    uint32_t trim_threshold = HZ4_TRANSFERCACHE_BATCH * 4;
    if (bin->count < trim_threshold) return;

    // keep サイズ: TRANSFERCACHE_BATCH (例: 32)
    uint32_t keep = HZ4_TRANSFERCACHE_BATCH;
    if (keep >= bin->count) return;

    // リストを 'keep' 位置で分割
    void* split_prev = NULL;
    void* cur = bin->head;
    for (uint32_t i = 0; i < keep && cur != NULL; i++) {
        split_prev = cur;
        cur = hz4_obj_get_next(cur);
    }

    if (cur == NULL) return;  // List shorter than expected

    void* excess_head = cur;
    void* excess_tail = NULL;
    uint32_t excess_count = 0;

    // Walk to tail
    void* walk = excess_head;
    while (walk) {
        excess_tail = walk;
        walk = hz4_obj_get_next(walk);
        excess_count++;
    }

    // Cut the list
    if (split_prev) {
        hz4_obj_set_next(split_prev, NULL);
    } else {
        bin->head = NULL;
    }
    bin->count = keep;

    // Push to tcbox (declared below)
    extern void hz4_tcbox_push_list(hz4_tls_t* tls, uint8_t sc,
                                    void* head, void* tail, uint32_t count);
    hz4_tcbox_push_list(tls, sc, excess_head, excess_tail, excess_count);
}

// ============================================================================
// Push list（バッチ化して Treiber stack へ）
// ============================================================================

void hz4_tcbox_push_list(hz4_tls_t* tls, uint8_t sc,
                         void* head, void* tail, uint32_t count) {
    (void)tail;  // Currently unused, reserved for future optimizations
    if (count == 0 || !head) return;

    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
    hz4_tcbox_stack_t* stack = &g_hz4_tcbox[owner % HZ4_TRANSFERCACHE_SHARDS][sc];

    void* batch_head;
    void* batch_tail;
    uint32_t batch_size;

    while (count > 0) {
        // Phase 20B-2: Use MIN macro for better codegen
        batch_size = TCBOX_MIN(count, HZ4_TRANSFERCACHE_BATCH);

        // Find batch split point (Phase 20B-3: move NULL check outside loop)
        batch_head = head;
        batch_tail = head;

        // Early exit for batch_size=1 (no loop needed)
        if (batch_size > 1) {
            for (uint32_t i = 1; i < batch_size; i++) {
                batch_tail = hz4_obj_get_next(batch_tail);
                if (!batch_tail) goto short_list;
            }
        }

        // Allocate batch node
        hz4_tcbox_batch_t* node = hz4_tcbox_batch_alloc(tls);
        if (!node) return;  // OOM fallback

        node->head = batch_head;
        node->tail = batch_tail;
        node->count = batch_size;
        node->sc = sc;

        // Cut batch from list (Phase 20B-4: eliminate redundant NULL check)
        head = hz4_obj_get_next(batch_tail);
        hz4_obj_set_next(batch_tail, NULL);
        count -= batch_size;

        // Treiber stack push
        hz4_tcbox_batch_t* old_head;
        do {
            old_head = atomic_load_explicit(&stack->head, memory_order_acquire);
            node->next = old_head;
        } while (!atomic_compare_exchange_weak_explicit(
                &stack->head, &old_head, node,
                memory_order_release, memory_order_acquire));
    }
    return;

short_list:
    // List shorter than expected, push remainder and exit
    if (batch_head) {
        hz4_tcbox_batch_t* node = hz4_tcbox_batch_alloc(tls);
        if (node) {
            node->head = batch_head;
            node->tail = batch_tail;
            node->count = batch_size;
            node->sc = sc;

            hz4_tcbox_batch_t* old_head;
            do {
                old_head = atomic_load_explicit(&stack->head, memory_order_acquire);
                node->next = old_head;
            } while (!atomic_compare_exchange_weak_explicit(
                    &stack->head, &old_head, node,
                    memory_order_release, memory_order_acquire));
        }
    }
}

// ============================================================================
// Pop（tcbox → bin）
// ============================================================================

static inline bool hz4_tcbox_pop(hz4_tls_t* tls, uint8_t sc, hz4_tcache_bin_t* bin) {
    if (!hz4_tcbox_pop_enabled(tls)) {
        return false;
    }
    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);
    hz4_tcbox_stack_t* stack = &g_hz4_tcbox[owner % HZ4_TRANSFERCACHE_SHARDS][sc];

    hz4_tcbox_batch_t* batch = atomic_load_explicit(&stack->head, memory_order_acquire);
    while (batch) {
        hz4_tcbox_batch_t* next = batch->next;
        if (atomic_compare_exchange_weak_explicit(
                &stack->head, &batch, next,
                memory_order_release, memory_order_acquire)) {
            // Popped successfully
            hz4_tcache_splice(bin, batch->head, batch->tail, batch->count);
            hz4_tcbox_batch_free(tls, batch);
            return true;
        }
        batch = atomic_load_explicit(&stack->head, memory_order_acquire);
    }

    return false;
}

#endif // HZ4_TRANSFERCACHE

#endif // HZ4_TCBOX_INC
