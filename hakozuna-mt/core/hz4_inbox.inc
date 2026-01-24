// hz4_inbox.inc - Inbox Box (MPSC per-owner/per-sc remote collection)
// Box Theory: segq/pageq を迂回して remote を inbox で一括回収
//
// 設計:
// - MPSC: multiple producers (remote flush), single consumer (collect)
// - owner x sc の inbox head
// - atomic_exchange で全取得 (drain)
// - 余りは TLS stash に退避

#ifndef HZ4_INBOX_INC
#define HZ4_INBOX_INC

#include "hz4_config.h"

#if HZ4_REMOTE_INBOX

#include "hz4_types.h"
#include "hz4_tls.h"

// ============================================================================
// Inbox Head (per owner x sc)
// ============================================================================
// _Alignas(64) で cacheline false sharing 回避
typedef struct {
    _Alignas(64) _Atomic(void*) head;
} hz4_inbox_head_t;

// グローバル inbox: extern 宣言 (実体は hz4_inbox.c)
extern hz4_inbox_head_t g_hz4_inbox[HZ4_NUM_SHARDS][HZ4_SC_MAX];

// ============================================================================
// Push List (MPSC - any thread)
// ============================================================================
// head..tail のリストを owner の inbox に追加
// 毎 iteration で tail.next を再設定 (CAS retry 対策)
static inline void hz4_inbox_push_list(uint8_t owner, uint8_t sc,
                                       void* head, void* tail) {
#if HZ4_FAILFAST
    if (owner >= HZ4_NUM_SHARDS) {
        HZ4_FAIL("inbox_push_list: owner out of range");
    }
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("inbox_push_list: sc out of range");
    }
#endif

    _Atomic(void*)* slot = &g_hz4_inbox[owner][sc].head;

    void* old = atomic_load_explicit(slot, memory_order_acquire);
    for (;;) {
        hz4_obj_set_next(tail, old);  // 毎回再設定 (CAS retry 対策)
        if (atomic_compare_exchange_weak_explicit(
                slot, &old, head,
                memory_order_release, memory_order_acquire)) {
            break;
        }
    }
}

// Push single object
static inline void hz4_inbox_push_one(uint8_t owner, uint8_t sc, void* obj) {
    hz4_inbox_push_list(owner, sc, obj, obj);
}

// ============================================================================
// Pop All (owner only)
// ============================================================================
// P3.1: 空 exchange 回避 - 空の inbox には atomic_exchange しない
static inline void* hz4_inbox_pop_all(uint8_t owner, uint8_t sc) {
#if HZ4_FAILFAST
    if (owner >= HZ4_NUM_SHARDS) {
        HZ4_FAIL("inbox_pop_all: owner out of range");
    }
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("inbox_pop_all: sc out of range");
    }
#endif

    _Atomic(void*)* slot = &g_hz4_inbox[owner][sc].head;

    // P3.1: 空チェック (relaxed で十分 - 判定用、同期不要)
    void* h = atomic_load_explicit(slot, memory_order_relaxed);
    if (__builtin_expect(h == NULL, 1)) return NULL;

    // 取りこぼし許容: load で non-NULL でも exchange が NULL を返す可能性あり
    // （別スレッドが先に取得）→ 次回 collect で回収されるため問題なし
    // exchange は acq_rel で list 取得の同期
    return atomic_exchange_explicit(slot, NULL, memory_order_acq_rel);
}

// ============================================================================
// Consume (collect 前段)
// ============================================================================
// inbox から objects を out[] に詰める
// 余りは TLS inbox_stash に退避 (pushback しない)
static inline uint32_t hz4_inbox_consume(hz4_tls_t* tls, uint8_t sc,
                                         void** out, uint32_t budget) {
#if HZ4_FAILFAST
    if (sc >= HZ4_SC_MAX) {
        HZ4_FAIL("inbox_consume: sc out of range");
    }
#endif

    uint8_t owner = (uint8_t)hz4_owner_shard(tls->tid);

    // First try TLS stash
    void* list = tls->inbox_stash[sc];
    if (list) {
        tls->inbox_stash[sc] = NULL;
    } else {
        // Pop from inbox
        list = hz4_inbox_pop_all(owner, sc);
    }

    if (!list) {
        return 0;
    }

    uint32_t got = 0;
    void* cur = list;

    // Walk list with budget
    while (cur && got < budget) {
        void* next = hz4_obj_get_next(cur);
        out[got++] = cur;
        cur = next;
    }

    // 余りは stash へ (pushback しない)
    if (cur) {
        tls->inbox_stash[sc] = cur;
    }

    return got;
}

#else  // !HZ4_REMOTE_INBOX

// Stubs when disabled
static inline void hz4_inbox_push_list(uint8_t owner, uint8_t sc,
                                       void* head, void* tail) {
    (void)owner; (void)sc; (void)head; (void)tail;
}

static inline void hz4_inbox_push_one(uint8_t owner, uint8_t sc, void* obj) {
    (void)owner; (void)sc; (void)obj;
}

static inline void* hz4_inbox_pop_all(uint8_t owner, uint8_t sc) {
    (void)owner; (void)sc;
    return NULL;
}

static inline uint32_t hz4_inbox_consume(hz4_tls_t* tls, uint8_t sc,
                                         void** out, uint32_t budget) {
    (void)tls; (void)sc; (void)out; (void)budget;
    return 0;
}

#endif  // HZ4_REMOTE_INBOX

#endif  // HZ4_INBOX_INC
