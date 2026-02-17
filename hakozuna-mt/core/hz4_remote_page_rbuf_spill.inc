// hz4_remote_page_rbuf_spill.inc - RemotePageRbufBox spill/message helpers
#ifndef HZ4_REMOTE_PAGE_RBUF_SPILL_INC
#define HZ4_REMOTE_PAGE_RBUF_SPILL_INC

#if HZ4_COLLECT_LIST
static inline void hz4_remote_idx_msg_list_splice(void** head,
                                                   void** tail,
                                                   void* h2,
                                                   void* t2) {
    if (!h2) return;
    if (!*head) {
        *head = h2;
        *tail = t2;
    } else {
        hz4_obj_set_next(*tail, h2);
        *tail = t2;
    }
}
#endif

#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
static inline hz4_remote_idx_msg_slot_t* hz4_remote_idx_msgpass_slot_from_code(uint8_t owner,
                                                                                 uint32_t code) {
    if (code == 0) {
        return NULL;
    }
    uint32_t prod = code - 1u;
    if (prod >= HZ4_NUM_SHARDS) {
        return NULL;
    }
    return &g_hz4_remote_idx_msg_slots[owner][prod];
}

static inline void hz4_remote_idx_msgpass_push_code(uint8_t owner, uint8_t sc, uint32_t code) {
    hz4_remote_idx_msg_slot_t* slot = hz4_remote_idx_msgpass_slot_from_code(owner, code);
    if (!slot) {
        return;
    }
    uint32_t old = atomic_load_explicit(&g_hz4_remote_idx_msg_heads[owner][sc], memory_order_acquire);
    do {
        slot->next_code = old;
    } while (!atomic_compare_exchange_weak_explicit(&g_hz4_remote_idx_msg_heads[owner][sc],
                                                     &old,
                                                     code,
                                                     memory_order_release,
                                                     memory_order_acquire));
}

static inline uint32_t hz4_remote_idx_msgpass_pop_all(uint8_t owner, uint8_t sc) {
    return atomic_exchange_explicit(&g_hz4_remote_idx_msg_heads[owner][sc], 0, memory_order_acq_rel);
}

static inline uint32_t hz4_remote_idx_msgpass_chain_tail(uint8_t owner, uint32_t head_code) {
    uint32_t tail = 0;
    uint32_t code = head_code;
    while (code) {
        hz4_remote_idx_msg_slot_t* slot = hz4_remote_idx_msgpass_slot_from_code(owner, code);
        if (!slot) {
            break;
        }
        tail = code;
        code = slot->next_code;
    }
    return tail;
}

static inline void hz4_remote_idx_msgpass_push_chain(uint8_t owner,
                                                      uint8_t sc,
                                                      uint32_t head_code,
                                                      uint32_t tail_code) {
    if (head_code == 0 || tail_code == 0) {
        return;
    }
    hz4_remote_idx_msg_slot_t* tail = hz4_remote_idx_msgpass_slot_from_code(owner, tail_code);
    if (!tail) {
        return;
    }
    uint32_t old = atomic_load_explicit(&g_hz4_remote_idx_msg_heads[owner][sc], memory_order_acquire);
    do {
        tail->next_code = old;
    } while (!atomic_compare_exchange_weak_explicit(&g_hz4_remote_idx_msg_heads[owner][sc],
                                                     &old,
                                                     head_code,
                                                     memory_order_release,
                                                     memory_order_acquire));
}

static inline bool hz4_remote_page_staging_msgpass_try_publish(hz4_tls_t* tls,
                                                                uint8_t owner,
                                                                uint8_t sc,
                                                                hz4_page_t* page,
                                                                const uint16_t* idxs,
                                                                uint16_t begin,
                                                                uint16_t n) {
    if (begin >= n) {
        return true;
    }
    uint16_t remain = (uint16_t)(n - begin);
    if (remain > HZ4_REMOTE_PAGE_STAGING_MAX) {
        return false;
    }
    if (remain < HZ4_REMOTE_PAGE_STAGING_META_MSGPASS_MIN_N) {
        return false;
    }
    uint8_t prod = (uint8_t)hz4_owner_shard(tls->tid);
    if ((uint32_t)prod >= HZ4_NUM_SHARDS) {
        return false;
    }
    uint32_t code = (uint32_t)prod + 1u;
    hz4_remote_idx_msg_slot_t* slot = &g_hz4_remote_idx_msg_slots[owner][prod];
    uint8_t expect = 0;
    if (!atomic_compare_exchange_strong_explicit(&slot->ready,
                                                 &expect,
                                                 2,
                                                 memory_order_acq_rel,
                                                 memory_order_acquire)) {
        return false;
    }
    slot->page = page;
    slot->sc = sc;
    slot->n = remain;
    for (uint16_t i = 0; i < remain; i++) {
        slot->idx[i] = idxs[(uint16_t)(begin + i)];
    }
    atomic_store_explicit(&slot->ready, 1, memory_order_release);
    hz4_remote_idx_msgpass_push_code(owner, sc, code);
    return true;
}

static inline uint32_t hz4_remote_idx_msgpass_consume_array(uint8_t owner,
                                                             uint8_t sc,
                                                             void** out,
                                                             uint32_t budget) {
    if (budget == 0) return 0;
    uint32_t got = 0;
    uint32_t code = hz4_remote_idx_msgpass_pop_all(owner, sc);
    uint32_t rem_head = 0;
    uint32_t rem_tail = 0;
    while (code) {
        if (got >= budget) {
            rem_head = code;
            rem_tail = hz4_remote_idx_msgpass_chain_tail(owner, rem_head);
            break;
        }
        hz4_remote_idx_msg_slot_t* slot = hz4_remote_idx_msgpass_slot_from_code(owner, code);
        if (!slot) {
            break;
        }
        uint32_t next_code = slot->next_code;
        if (atomic_load_explicit(&slot->ready, memory_order_acquire) == 0) {
            code = next_code;
            continue;
        }
        uint8_t slot_sc = slot->sc;
        if (slot_sc != sc) {
            hz4_remote_idx_msgpass_push_code(owner, slot_sc, code);
            code = next_code;
            continue;
        }
        uint16_t n = slot->n;
        if (n == 0) {
            atomic_store_explicit(&slot->ready, 0, memory_order_release);
            code = next_code;
            continue;
        }
        if ((uint32_t)n > (budget - got)) {
            rem_head = code;
            rem_tail = hz4_remote_idx_msgpass_chain_tail(owner, rem_head);
            break;
        }

        hz4_page_t* page = slot->page;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        if (hz4_page_valid(page) && meta->sc == sc && meta->capacity > 0) {
            size_t obj_size = hz4_sc_to_size(sc);
            uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
            uint16_t dec = 0;
            for (uint16_t i = 0; i < n; i++) {
                uint16_t idx = slot->idx[i];
                if (idx >= meta->capacity) {
                    continue;
                }
                out[got++] = (void*)(start + ((uintptr_t)idx * obj_size));
                dec++;
            }
            if (dec > 0) {
                hz4_page_used_dec_meta(meta, dec);
            }
        }

        atomic_store_explicit(&slot->ready, 0, memory_order_release);
        code = next_code;
    }
    if (rem_head && rem_tail) {
        hz4_remote_idx_msgpass_push_chain(owner, sc, rem_head, rem_tail);
    }
    return got;
}

#if HZ4_COLLECT_LIST
static inline uint32_t hz4_remote_idx_msgpass_consume_list(uint8_t owner,
                                                            uint8_t sc,
                                                            void** head_out,
                                                            void** tail_out,
                                                            uint32_t budget) {
    *head_out = NULL;
    *tail_out = NULL;
    if (budget == 0) return 0;

    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;
    uint32_t code = hz4_remote_idx_msgpass_pop_all(owner, sc);
    uint32_t rem_head = 0;
    uint32_t rem_tail = 0;
    while (code) {
        if (got >= budget) {
            rem_head = code;
            rem_tail = hz4_remote_idx_msgpass_chain_tail(owner, rem_head);
            break;
        }
        hz4_remote_idx_msg_slot_t* slot = hz4_remote_idx_msgpass_slot_from_code(owner, code);
        if (!slot) {
            break;
        }
        uint32_t next_code = slot->next_code;
        if (atomic_load_explicit(&slot->ready, memory_order_acquire) == 0) {
            code = next_code;
            continue;
        }
        uint8_t slot_sc = slot->sc;
        if (slot_sc != sc) {
            hz4_remote_idx_msgpass_push_code(owner, slot_sc, code);
            code = next_code;
            continue;
        }
        uint16_t n = slot->n;
        if (n == 0) {
            atomic_store_explicit(&slot->ready, 0, memory_order_release);
            code = next_code;
            continue;
        }
        if ((uint32_t)n > (budget - got)) {
            rem_head = code;
            rem_tail = hz4_remote_idx_msgpass_chain_tail(owner, rem_head);
            break;
        }

        hz4_page_t* page = slot->page;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        if (hz4_page_valid(page) && meta->sc == sc && meta->capacity > 0) {
            size_t obj_size = hz4_sc_to_size(sc);
            uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
            uint16_t dec = 0;
            for (uint16_t i = 0; i < n; i++) {
                uint16_t idx = slot->idx[i];
                if (idx >= meta->capacity) {
                    continue;
                }
                void* obj = (void*)(start + ((uintptr_t)idx * obj_size));
                hz4_obj_set_next(obj, NULL);
                if (!head) {
                    head = obj;
                    tail = obj;
                } else {
                    hz4_obj_set_next(tail, obj);
                    tail = obj;
                }
                got++;
                dec++;
            }
            if (dec > 0) {
                hz4_page_used_dec_meta(meta, dec);
            }
        }

        atomic_store_explicit(&slot->ready, 0, memory_order_release);
        code = next_code;
    }
    if (rem_head && rem_tail) {
        hz4_remote_idx_msgpass_push_chain(owner, sc, rem_head, rem_tail);
    }
    *head_out = head;
    *tail_out = tail;
    return got;
}
#endif

#else
static inline bool hz4_remote_page_staging_msgpass_try_publish(hz4_tls_t* tls,
                                                                uint8_t owner,
                                                                uint8_t sc,
                                                                hz4_page_t* page,
                                                                const uint16_t* idxs,
                                                                uint16_t begin,
                                                                uint16_t n) {
    (void)tls;
    (void)owner;
    (void)sc;
    (void)page;
    (void)idxs;
    (void)begin;
    (void)n;
    return false;
}
static inline uint32_t hz4_remote_idx_msgpass_consume_array(uint8_t owner,
                                                             uint8_t sc,
                                                             void** out,
                                                             uint32_t budget) {
    (void)owner;
    (void)sc;
    (void)out;
    (void)budget;
    return 0;
}
#if HZ4_COLLECT_LIST
static inline uint32_t hz4_remote_idx_msgpass_consume_list(uint8_t owner,
                                                            uint8_t sc,
                                                            void** head_out,
                                                            void** tail_out,
                                                            uint32_t budget) {
    (void)owner;
    (void)sc;
    (void)head_out;
    (void)tail_out;
    (void)budget;
    return 0;
}
#endif
#endif

#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
static inline void hz4_remote_idx_msg_pool_init_once(void) {
    if (atomic_load_explicit(&g_hz4_remote_idx_msg_pool_ready, memory_order_acquire)) {
        return;
    }
    if (!atomic_flag_test_and_set_explicit(&g_hz4_remote_idx_msg_pool_lock, memory_order_acquire)) {
        if (!atomic_load_explicit(&g_hz4_remote_idx_msg_pool_ready, memory_order_relaxed)) {
            for (uint32_t i = 0; i < HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N; i++) {
                g_hz4_remote_idx_msg_nodes[i].next =
                    (i + 1 < HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_POOL_N)
                        ? &g_hz4_remote_idx_msg_nodes[i + 1]
                        : NULL;
            }
            atomic_store_explicit(&g_hz4_remote_idx_msg_free,
                                  &g_hz4_remote_idx_msg_nodes[0],
                                  memory_order_release);
            atomic_store_explicit(&g_hz4_remote_idx_msg_pool_ready, 1, memory_order_release);
        }
        atomic_flag_clear_explicit(&g_hz4_remote_idx_msg_pool_lock, memory_order_release);
        return;
    }
    while (!atomic_load_explicit(&g_hz4_remote_idx_msg_pool_ready, memory_order_acquire)) {
    }
}

static inline hz4_remote_idx_msg_node_t* hz4_remote_idx_msg_alloc(void) {
    hz4_remote_idx_msg_pool_init_once();
    if (t_hz4_remote_idx_msg_cache == NULL) {
        hz4_remote_idx_msg_node_t* head =
            atomic_load_explicit(&g_hz4_remote_idx_msg_free, memory_order_acquire);
        while (head) {
            hz4_remote_idx_msg_node_t* tail = head;
            uint16_t take = 1;
            while (take < HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N && tail->next) {
                tail = tail->next;
                take++;
            }
            hz4_remote_idx_msg_node_t* next = tail->next;
            if (atomic_compare_exchange_weak_explicit(&g_hz4_remote_idx_msg_free,
                                                      &head,
                                                      next,
                                                      memory_order_acq_rel,
                                                      memory_order_acquire)) {
                tail->next = NULL;
                t_hz4_remote_idx_msg_cache = head;
                t_hz4_remote_idx_msg_cache_n = take;
                break;
            }
        }
    }
    hz4_remote_idx_msg_node_t* node = t_hz4_remote_idx_msg_cache;
    if (node) {
        t_hz4_remote_idx_msg_cache = node->next;
        node->next = NULL;
        if (t_hz4_remote_idx_msg_cache_n > 0) {
            t_hz4_remote_idx_msg_cache_n--;
        }
        return node;
    }
    return NULL;
}

static inline void hz4_remote_idx_msg_free_push_list(hz4_remote_idx_msg_node_t* head,
                                                      hz4_remote_idx_msg_node_t* tail) {
    if (!head || !tail) {
        return;
    }
    hz4_remote_idx_msg_node_t* old =
        atomic_load_explicit(&g_hz4_remote_idx_msg_free, memory_order_acquire);
    do {
        tail->next = old;
    } while (!atomic_compare_exchange_weak_explicit(&g_hz4_remote_idx_msg_free,
                                                    &old,
                                                    head,
                                                    memory_order_acq_rel,
                                                    memory_order_acquire));
}

static inline void hz4_remote_idx_msg_free_one(hz4_remote_idx_msg_node_t* node) {
    node->next = t_hz4_remote_idx_msg_cache;
    t_hz4_remote_idx_msg_cache = node;
    if (t_hz4_remote_idx_msg_cache_n < UINT16_MAX) {
        t_hz4_remote_idx_msg_cache_n++;
    }
    uint16_t limit =
        (uint16_t)(HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N * 2u);
    if (t_hz4_remote_idx_msg_cache_n < limit) {
        return;
    }
    hz4_remote_idx_msg_node_t* head = t_hz4_remote_idx_msg_cache;
    hz4_remote_idx_msg_node_t* tail = head;
    uint16_t give = 1;
    uint16_t target = HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_TLS_CACHE_N;
    while (give < target && tail->next) {
        tail = tail->next;
        give++;
    }
    t_hz4_remote_idx_msg_cache = tail->next;
    tail->next = NULL;
    t_hz4_remote_idx_msg_cache_n = (uint16_t)(t_hz4_remote_idx_msg_cache_n - give);
    hz4_remote_idx_msg_free_push_list(head, tail);
}

static inline void hz4_remote_idx_msgq_push_list(uint8_t owner,
                                                  uint8_t sc,
                                                  hz4_remote_idx_msg_node_t* head,
                                                  hz4_remote_idx_msg_node_t* tail) {
    if (!head) {
        return;
    }
    hz4_remote_idx_msg_node_t* old =
        atomic_load_explicit(&g_hz4_remote_idx_msgq[owner][sc], memory_order_acquire);
    do {
        tail->next = old;
    } while (!atomic_compare_exchange_weak_explicit(&g_hz4_remote_idx_msgq[owner][sc],
                                                    &old,
                                                    head,
                                                    memory_order_release,
                                                    memory_order_acquire));
}

static inline hz4_remote_idx_msg_node_t* hz4_remote_idx_msgq_pop_all(uint8_t owner, uint8_t sc) {
    hz4_remote_idx_msg_node_t* head =
        atomic_load_explicit(&g_hz4_remote_idx_msgq[owner][sc], memory_order_acquire);
    if (!head) {
        return NULL;
    }
    return atomic_exchange_explicit(&g_hz4_remote_idx_msgq[owner][sc], NULL, memory_order_acq_rel);
}

static inline bool hz4_remote_page_staging_spill_msg_try_publish(uint8_t owner,
                                                                  uint8_t sc,
                                                                  hz4_page_t* page,
                                                                  const uint16_t* idxs,
                                                                  uint16_t begin,
                                                                  uint16_t n) {
    if (begin >= n) {
        return true;
    }
    uint16_t remain = (uint16_t)(n - begin);
    if (remain > HZ4_REMOTE_PAGE_STAGING_MAX) {
        return false;
    }
    if (remain < HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG_MIN_N) {
        return false;
    }

    hz4_remote_idx_msg_node_t* node = hz4_remote_idx_msg_alloc();
    if (!node) {
        return false;
    }

    node->page = page;
    node->n = remain;
    node->sc = sc;
    for (uint16_t i = 0; i < remain; i++) {
        node->idx[i] = idxs[(uint16_t)(begin + i)];
    }
    hz4_remote_idx_msgq_push_list(owner, sc, node, node);
    return true;
}

static inline uint32_t hz4_remote_idx_msgq_consume_array_spill(uint8_t owner,
                                                                uint8_t sc,
                                                                void** out,
                                                                uint32_t budget) {
    if (budget == 0) {
        return 0;
    }
    hz4_remote_idx_msg_node_t* node = hz4_remote_idx_msgq_pop_all(owner, sc);
    hz4_remote_idx_msg_node_t* rem_head = NULL;
    hz4_remote_idx_msg_node_t* rem_tail = NULL;
    uint32_t got = 0;
    while (node) {
        hz4_remote_idx_msg_node_t* next = node->next;
        node->next = NULL;
        uint16_t n = node->n;
        if ((uint32_t)n > (budget - got)) {
            node->next = next;
            rem_head = node;
            hz4_remote_idx_msg_node_t* tail = node;
            while (tail->next) {
                tail = tail->next;
            }
            rem_tail = tail;
            break;
        }

        hz4_page_t* page = node->page;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        if (hz4_page_valid(page) && meta->sc == sc && meta->capacity > 0) {
            size_t obj_size = hz4_sc_to_size(sc);
            uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
            uint16_t dec = 0;
            for (uint16_t i = 0; i < n; i++) {
                uint16_t idx = node->idx[i];
                if (idx >= meta->capacity) {
                    continue;
                }
                out[got++] = (void*)(start + ((uintptr_t)idx * obj_size));
                dec++;
            }
            if (dec > 0) {
                hz4_page_used_dec_meta(meta, dec);
            }
        }
        hz4_remote_idx_msg_free_one(node);
        node = next;
    }

    if (rem_head) {
        hz4_remote_idx_msgq_push_list(owner, sc, rem_head, rem_tail);
    }
    return got;
}

#if HZ4_COLLECT_LIST
static inline uint32_t hz4_remote_idx_msgq_consume_list_spill(uint8_t owner,
                                                               uint8_t sc,
                                                               void** head_out,
                                                               void** tail_out,
                                                               uint32_t budget) {
    *head_out = NULL;
    *tail_out = NULL;
    if (budget == 0) {
        return 0;
    }
    hz4_remote_idx_msg_node_t* node = hz4_remote_idx_msgq_pop_all(owner, sc);
    hz4_remote_idx_msg_node_t* rem_head = NULL;
    hz4_remote_idx_msg_node_t* rem_tail = NULL;
    void* head = NULL;
    void* tail = NULL;
    uint32_t got = 0;
    while (node) {
        hz4_remote_idx_msg_node_t* next = node->next;
        node->next = NULL;
        uint16_t n = node->n;
        if ((uint32_t)n > (budget - got)) {
            node->next = next;
            rem_head = node;
            hz4_remote_idx_msg_node_t* tail_node = node;
            while (tail_node->next) {
                tail_node = tail_node->next;
            }
            rem_tail = tail_node;
            break;
        }

        hz4_page_t* page = node->page;
        hz4_page_meta_t* meta = hz4_page_meta(page);
        if (hz4_page_valid(page) && meta->sc == sc && meta->capacity > 0) {
            size_t obj_size = hz4_sc_to_size(sc);
            uintptr_t start = (uintptr_t)page + hz4_align_up(sizeof(hz4_page_t), HZ4_SIZE_ALIGN);
            uint16_t dec = 0;
            for (uint16_t i = 0; i < n; i++) {
                uint16_t idx = node->idx[i];
                if (idx >= meta->capacity) {
                    continue;
                }
                void* obj = (void*)(start + ((uintptr_t)idx * obj_size));
                hz4_obj_set_next(obj, NULL);
                if (!head) {
                    head = obj;
                    tail = obj;
                } else {
                    hz4_obj_set_next(tail, obj);
                    tail = obj;
                }
                got++;
                dec++;
            }
            if (dec > 0) {
                hz4_page_used_dec_meta(meta, dec);
            }
        }
        hz4_remote_idx_msg_free_one(node);
        node = next;
    }

    if (rem_head) {
        hz4_remote_idx_msgq_push_list(owner, sc, rem_head, rem_tail);
    }
    *head_out = head;
    *tail_out = tail;
    return got;
}
#endif
#else
static inline bool hz4_remote_page_staging_spill_msg_try_publish(uint8_t owner,
                                                                  uint8_t sc,
                                                                  hz4_page_t* page,
                                                                  const uint16_t* idxs,
                                                                  uint16_t begin,
                                                                  uint16_t n) {
    (void)owner;
    (void)sc;
    (void)page;
    (void)idxs;
    (void)begin;
    (void)n;
    return false;
}

static inline uint32_t hz4_remote_idx_msgq_consume_array_spill(uint8_t owner,
                                                                uint8_t sc,
                                                                void** out,
                                                                uint32_t budget) {
    (void)owner;
    (void)sc;
    (void)out;
    (void)budget;
    return 0;
}
#if HZ4_COLLECT_LIST
static inline uint32_t hz4_remote_idx_msgq_consume_list_spill(uint8_t owner,
                                                               uint8_t sc,
                                                               void** head_out,
                                                               void** tail_out,
                                                               uint32_t budget) {
    (void)owner;
    (void)sc;
    (void)head_out;
    (void)tail_out;
    (void)budget;
    return 0;
}
#endif
#endif

static inline uint32_t hz4_remote_idx_msgq_consume_array(uint8_t owner,
                                                          uint8_t sc,
                                                          void** out,
                                                          uint32_t budget) {
    if (budget == 0) return 0;
#if !HZ4_REMOTE_PAGE_STAGING_META_MSGPASS && !HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
    (void)owner;
    (void)sc;
    (void)out;
#endif
    uint32_t got = 0;
#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
    got += hz4_remote_idx_msgpass_consume_array(owner, sc, out + got, budget - got);
#endif
#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
    if (got < budget) {
        got += hz4_remote_idx_msgq_consume_array_spill(owner, sc, out + got, budget - got);
    }
#endif
    return got;
}

#if HZ4_COLLECT_LIST
static inline uint32_t hz4_remote_idx_msgq_consume_list(uint8_t owner,
                                                         uint8_t sc,
                                                         void** head_out,
                                                         void** tail_out,
                                                         uint32_t budget) {
    *head_out = NULL;
    *tail_out = NULL;
    if (budget == 0) return 0;
#if !HZ4_REMOTE_PAGE_STAGING_META_MSGPASS && !HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
    (void)owner;
    (void)sc;
#endif
    uint32_t got = 0;

#if HZ4_REMOTE_PAGE_STAGING_META_MSGPASS
    void* mh = NULL;
    void* mt = NULL;
    uint32_t msg_got = hz4_remote_idx_msgpass_consume_list(owner, sc, &mh, &mt, budget - got);
    if (msg_got > 0) {
        hz4_remote_idx_msg_list_splice(head_out, tail_out, mh, mt);
        got += msg_got;
    }
#endif

#if HZ4_REMOTE_PAGE_STAGING_META_SPILL_MSG
    if (got < budget) {
        void* sh = NULL;
        void* st = NULL;
        uint32_t spill_got = hz4_remote_idx_msgq_consume_list_spill(owner, sc, &sh, &st, budget - got);
        if (spill_got > 0) {
            hz4_remote_idx_msg_list_splice(head_out, tail_out, sh, st);
            got += spill_got;
        }
    }
#endif
    return got;
}
#endif

#endif // HZ4_REMOTE_PAGE_RBUF_SPILL_INC
