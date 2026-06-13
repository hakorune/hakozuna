#if HZ3_S282_LARGE_OWNER_INBOX_OBS
static inline void hz3_s282_owner_inbox_inc(_Atomic size_t* p) {
    hz3_large_s240_inc(p);
}

static inline void hz3_s282_owner_inbox_add(_Atomic size_t* p, size_t v) {
    hz3_large_s240_add(p, v);
}

static inline void hz3_s282_owner_inbox_depth_bucket(size_t depth,
                                                     _Atomic size_t* depth_0,
                                                     _Atomic size_t* depth_1,
                                                     _Atomic size_t* depth_2_3,
                                                     _Atomic size_t* depth_4_7,
                                                     _Atomic size_t* depth_8_15,
                                                     _Atomic size_t* depth_16_plus) {
    if (depth == 0) {
        hz3_s282_owner_inbox_inc(depth_0);
    } else if (depth == 1) {
        hz3_s282_owner_inbox_inc(depth_1);
    } else if (depth <= 3) {
        hz3_s282_owner_inbox_inc(depth_2_3);
    } else if (depth <= 7) {
        hz3_s282_owner_inbox_inc(depth_4_7);
    } else if (depth <= 15) {
        hz3_s282_owner_inbox_inc(depth_8_15);
    } else {
        hz3_s282_owner_inbox_inc(depth_16_plus);
    }
}

static inline void hz3_s282_owner_inbox_on_push_attempt(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_attempt);
}

static inline void hz3_s282_owner_inbox_on_push_ok(size_t cas_failures) {
    hz3_s282_owner_inbox_inc(&g_s282_push_ok);
    hz3_large_s240_update_max(&g_s282_push_cas_failure_max_per_push,
                              cas_failures);
}

static inline void hz3_s282_owner_inbox_on_push_reject_invalid(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_invalid);
}

static inline void hz3_s282_owner_inbox_on_push_reject_owner_dead(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_owner_dead);
}

static inline void hz3_s282_owner_inbox_on_push_reject_owner_exiting(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_owner_exiting);
}

static inline void hz3_s282_owner_inbox_on_push_reject_shard_collision(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_shard_collision);
}

static inline void hz3_s282_owner_inbox_on_push_reject_budget(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_reject_budget);
}

static inline void hz3_s282_owner_inbox_on_push_depth(size_t depth) {
    hz3_s282_owner_inbox_depth_bucket(depth,
                                      &g_s282_push_depth_0,
                                      &g_s282_push_depth_1,
                                      &g_s282_push_depth_2_3,
                                      &g_s282_push_depth_4_7,
                                      &g_s282_push_depth_8_15,
                                      &g_s282_push_depth_16_plus);
}

static inline void hz3_s282_owner_inbox_on_push_cas_attempt(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_cas_attempts);
}

static inline void hz3_s282_owner_inbox_on_push_cas_failure(void) {
    hz3_s282_owner_inbox_inc(&g_s282_push_cas_failures);
}

static inline void hz3_s282_owner_inbox_on_pop_cas_attempt(void) {
    hz3_s282_owner_inbox_inc(&g_s282_pop_cas_attempts);
}

static inline void hz3_s282_owner_inbox_on_pop_cas_failure(void) {
    hz3_s282_owner_inbox_inc(&g_s282_pop_cas_failures);
}

static inline void hz3_s282_owner_inbox_on_pop_success(size_t cas_failures) {
    hz3_s282_owner_inbox_inc(&g_s282_pop_success);
    hz3_large_s240_update_max(&g_s282_pop_cas_failure_max_per_pop,
                              cas_failures);
}

static inline void hz3_s282_owner_inbox_on_pop_empty(int saw_head) {
    hz3_s282_owner_inbox_inc(saw_head ? &g_s282_pop_race_empty :
                                        &g_s282_pop_empty);
}

static inline void hz3_s282_owner_inbox_on_take_first_call(size_t depth) {
    hz3_s282_owner_inbox_inc(&g_s282_take_first_calls);
    hz3_s282_owner_inbox_depth_bucket(depth,
                                      &g_s282_take_depth_0,
                                      &g_s282_take_depth_1,
                                      &g_s282_take_depth_2_3,
                                      &g_s282_take_depth_4_7,
                                      &g_s282_take_depth_8_15,
                                      &g_s282_take_depth_16_plus);
}

static inline void hz3_s282_owner_inbox_on_take_first_hit(void) {
    hz3_s282_owner_inbox_inc(&g_s282_take_first_hit);
    hz3_s282_owner_inbox_inc(&g_s282_drain_return_first_nodes);
}

static inline void hz3_s282_owner_inbox_on_take_first_miss(size_t depth) {
    hz3_s282_owner_inbox_inc(depth == 0 ? &g_s282_take_first_miss_empty :
                                          &g_s282_take_first_miss_race);
}

static inline void hz3_s282_owner_inbox_on_drain_nodes(size_t nodes) {
    hz3_s282_owner_inbox_add(&g_s282_drain_nodes, nodes);
}

static inline void hz3_s282_owner_inbox_on_extra_fronted(void) {
    hz3_s282_owner_inbox_inc(&g_s282_drain_to_front_nodes);
}

static inline void hz3_s282_owner_inbox_on_extra_munmap(void) {
    hz3_s282_owner_inbox_inc(&g_s282_drain_to_munmap_nodes);
}
#else
static inline void hz3_s282_owner_inbox_on_push_attempt(void) {}
static inline void hz3_s282_owner_inbox_on_push_ok(size_t cas_failures) {
    (void)cas_failures;
}
static inline void hz3_s282_owner_inbox_on_push_reject_invalid(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_owner_dead(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_owner_exiting(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_shard_collision(void) {}
static inline void hz3_s282_owner_inbox_on_push_reject_budget(void) {}
static inline void hz3_s282_owner_inbox_on_push_depth(size_t depth) {
    (void)depth;
}
static inline void hz3_s282_owner_inbox_on_push_cas_attempt(void) {}
static inline void hz3_s282_owner_inbox_on_push_cas_failure(void) {}
static inline void hz3_s282_owner_inbox_on_pop_cas_attempt(void) {}
static inline void hz3_s282_owner_inbox_on_pop_cas_failure(void) {}
static inline void hz3_s282_owner_inbox_on_pop_success(size_t cas_failures) {
    (void)cas_failures;
}
static inline void hz3_s282_owner_inbox_on_pop_empty(int saw_head) {
    (void)saw_head;
}
static inline void hz3_s282_owner_inbox_on_take_first_call(size_t depth) {
    (void)depth;
}
static inline void hz3_s282_owner_inbox_on_take_first_hit(void) {}
static inline void hz3_s282_owner_inbox_on_take_first_miss(size_t depth) {
    (void)depth;
}
static inline void hz3_s282_owner_inbox_on_drain_nodes(size_t nodes) {
    (void)nodes;
}
static inline void hz3_s282_owner_inbox_on_extra_fronted(void) {}
static inline void hz3_s282_owner_inbox_on_extra_munmap(void) {}
#endif

#if HZ3_S285_LARGE_FIXED_COST_OBS
static inline void hz3_s285_inc(_Atomic size_t* p) {
    hz3_large_s240_inc(p);
}

static inline void hz3_s285_on_direct_way_load(void) {
    hz3_s285_inc(&g_s285_direct_way_loads);
}

static inline void hz3_s285_on_direct_key_busy(void) {
    hz3_s285_inc(&g_s285_direct_key_busy);
}

static inline void hz3_s285_on_direct_key_miss(void) {
    hz3_s285_inc(&g_s285_direct_key_miss);
}

static inline void hz3_s285_on_direct_key_match(void) {
    hz3_s285_inc(&g_s285_direct_key_match);
}

static inline void hz3_s285_on_direct_invalid_hdr(void) {
    hz3_s285_inc(&g_s285_direct_invalid_hdr);
}

static inline void hz3_s285_on_direct_state_nonactive(void) {
    hz3_s285_inc(&g_s285_direct_state_nonactive);
}

static inline void hz3_s285_on_direct_inuse_bad(void) {
    hz3_s285_inc(&g_s285_direct_inuse_bad);
}

static inline void hz3_s285_on_direct_not_retainable(int same_owner) {
    hz3_s285_inc(same_owner ? &g_s285_direct_not_retainable_same_owner :
                              &g_s285_direct_not_retainable_remote);
}

static inline void hz3_s285_on_direct_cas_attempt(void) {
    hz3_s285_inc(&g_s285_direct_cas_attempt);
}

static inline void hz3_s285_on_direct_cas_fail(void) {
    hz3_s285_inc(&g_s285_direct_cas_fail);
}

static inline void hz3_s285_on_direct_hit(int inbox) {
    hz3_s285_inc(inbox ? &g_s285_direct_hit_inbox : &g_s285_direct_hit_front);
}

static inline void hz3_s285_on_activate_check(void) {
    hz3_s285_inc(&g_s285_activate_check);
}

static inline void hz3_s285_on_activate_fail_header(void) {
    hz3_s285_inc(&g_s285_activate_fail_header);
}

static inline void hz3_s285_on_activate_fail_slot(void) {
    hz3_s285_inc(&g_s285_activate_fail_slot);
}

static inline void hz3_s285_on_activate_fail_flag(void) {
    hz3_s285_inc(&g_s285_activate_fail_flag);
}

static inline void hz3_s285_on_activate_fail_sc(void) {
    hz3_s285_inc(&g_s285_activate_fail_sc);
}

static inline void hz3_s285_on_activate_fail_alignment(void) {
    hz3_s285_inc(&g_s285_activate_fail_alignment);
}

static inline void hz3_s285_on_activate_fail_user_alignment(void) {
    hz3_s285_inc(&g_s285_activate_fail_user_alignment);
}

static inline void hz3_s285_on_activate_fail_range(void) {
    hz3_s285_inc(&g_s285_activate_fail_range);
}

static inline void hz3_s285_on_activate_fail_size(void) {
    hz3_s285_inc(&g_s285_activate_fail_size);
}

static inline void hz3_s285_on_activate_success(int inbox) {
    hz3_s285_inc(inbox ? &g_s285_activate_success_inbox :
                         &g_s285_activate_success_front);
}

static inline void hz3_s285_on_take_first_return_first(void) {
    hz3_s285_inc(&g_s285_take_first_return_first);
}

static inline void hz3_s285_on_take_first_extra_pop_attempt(void) {
    hz3_s285_inc(&g_s285_take_first_extra_pop_attempt);
}

static inline void hz3_s285_on_take_first_extra_pop_hit(void) {
    hz3_s285_inc(&g_s285_take_first_extra_pop_hit);
}

static inline void hz3_s285_on_take_first_extra_pop_empty(void) {
    hz3_s285_inc(&g_s285_take_first_extra_pop_empty);
}

static inline void hz3_s285_on_take_first_extra_front_result(int ok) {
    hz3_s285_inc(ok ? &g_s285_take_first_extra_front_ok :
                     &g_s285_take_first_extra_front_reject);
}

static inline void hz3_s285_on_take_first_drain_nodes(int drained) {
    if (drained <= 1) {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_1);
    } else if (drained <= 3) {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_2_3);
    } else if (drained <= 7) {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_4_7);
    } else {
        hz3_s285_inc(&g_s285_take_first_drain_nodes_8_plus);
    }
}
#else
static inline void hz3_s285_on_direct_way_load(void) {}
static inline void hz3_s285_on_direct_key_busy(void) {}
static inline void hz3_s285_on_direct_key_miss(void) {}
static inline void hz3_s285_on_direct_key_match(void) {}
static inline void hz3_s285_on_direct_invalid_hdr(void) {}
static inline void hz3_s285_on_direct_state_nonactive(void) {}
static inline void hz3_s285_on_direct_inuse_bad(void) {}
static inline void hz3_s285_on_direct_not_retainable(int same_owner) {
    (void)same_owner;
}
static inline void hz3_s285_on_direct_cas_attempt(void) {}
static inline void hz3_s285_on_direct_cas_fail(void) {}
static inline void hz3_s285_on_direct_hit(int inbox) {
    (void)inbox;
}
static inline void hz3_s285_on_activate_check(void) {}
static inline void hz3_s285_on_activate_fail_header(void) {}
static inline void hz3_s285_on_activate_fail_slot(void) {}
static inline void hz3_s285_on_activate_fail_flag(void) {}
static inline void hz3_s285_on_activate_fail_sc(void) {}
static inline void hz3_s285_on_activate_fail_alignment(void) {}
static inline void hz3_s285_on_activate_fail_user_alignment(void) {}
static inline void hz3_s285_on_activate_fail_range(void) {}
static inline void hz3_s285_on_activate_fail_size(void) {}
static inline void hz3_s285_on_activate_success(int inbox) {
    (void)inbox;
}
static inline void hz3_s285_on_take_first_return_first(void) {}
static inline void hz3_s285_on_take_first_extra_pop_attempt(void) {}
static inline void hz3_s285_on_take_first_extra_pop_hit(void) {}
static inline void hz3_s285_on_take_first_extra_pop_empty(void) {}
static inline void hz3_s285_on_take_first_extra_front_result(int ok) {
    (void)ok;
}
static inline void hz3_s285_on_take_first_drain_nodes(int drained) {
    (void)drained;
}
#endif

#if HZ3_S251_LARGE_OWNER_FRONT_SAFETY
static inline void hz3_large_s251_fail(const char* where,
                                       Hz3LargeHdr* hdr,
                                       uint32_t expected,
                                       uint32_t actual,
                                       _Atomic size_t* counter) {
    hz3_large_s240_register_once();
    hz3_large_s240_inc(counter);
#if HZ3_S251_LARGE_OWNER_FRONT_FAILFAST
    fprintf(stderr,
            "[HZ3_LARGE_S251_FAILFAST] where=%s hdr=%p expected=%u actual=%u "
            "map_base=%p map_size=%zu user_ptr=%p owner=%u sc=%u flags=0x%x\n",
            where,
            (void*)hdr,
            expected,
            actual,
            hdr ? hdr->map_base : NULL,
            hdr ? hdr->map_size : 0,
            hdr ? hdr->user_ptr : NULL,
            hdr ? (unsigned)hdr->owner_shard : 255u,
            hdr ? (unsigned)hdr->sc : 255u,
            hdr ? (unsigned)hdr->flags : 0u);
    abort();
#else
    (void)where;
    (void)hdr;
    (void)expected;
    (void)actual;
#endif
}

static inline void hz3_large_s251_transition(Hz3LargeHdr* hdr,
                                             uint32_t expected,
                                             uint32_t desired,
                                             const char* where,
                                             _Atomic size_t* ok_counter,
                                             _Atomic size_t* fail_counter) {
    if (!hdr) {
        hz3_large_s251_fail(where, hdr, expected, 0, fail_counter);
        return;
    }
    uint32_t old = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (old != expected) {
        hz3_large_s251_fail(where, hdr, expected, old, fail_counter);
    }
    atomic_store_explicit(&hdr->state, desired, memory_order_release);
    hz3_large_s240_inc(ok_counter);
}

static inline void hz3_large_s251_expect(Hz3LargeHdr* hdr,
                                         uint32_t expected,
                                         const char* where,
                                         _Atomic size_t* fail_counter) {
    if (!hdr) {
        hz3_large_s251_fail(where, hdr, expected, 0, fail_counter);
        return;
    }
    uint32_t old = atomic_load_explicit(&hdr->state, memory_order_acquire);
    if (old != expected) {
        hz3_large_s251_fail(where, hdr, expected, old, fail_counter);
    }
}

static inline void hz3_large_s251_on_global_pop(Hz3LargeHdr* hdr) {
    hz3_large_s251_transition(hdr,
                              HZ3_LARGE_STATE_GLOBAL,
                              HZ3_LARGE_STATE_FREEING,
                              "global_pop",
                              &g_s251_global_pop_to_freeing,
                              &g_s251_global_pop_state_fail);
}
#else
static inline void hz3_large_s251_on_global_pop(Hz3LargeHdr* hdr) {
    (void)hdr;
}
#endif

