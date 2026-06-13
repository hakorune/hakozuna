static inline size_t hz3_large_aligned_obs_timing_start(void) {
    return 0;
}
static inline void hz3_large_aligned_obs_on_map_insert_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_large_aligned_obs_on_map_take_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_large_aligned_obs_on_alloc_cache_pop_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_large_aligned_obs_on_free_admit_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline size_t hz3_s243_residual_timing_start(void) { return 0; }
static inline void hz3_s243_residual_on_alloc_front_first_elapsed(size_t start_ns,
                                                                   int hit) {
    (void)start_ns; (void)hit;
}
static inline void hz3_s243_residual_on_alloc_inbox_drain_elapsed(size_t start_ns,
                                                                   int drained) {
    (void)start_ns; (void)drained;
}
static inline void hz3_s243_residual_on_alloc_front_after_drain_elapsed(size_t start_ns,
                                                                        int hit) {
    (void)start_ns; (void)hit;
}
static inline void hz3_s243_residual_on_alloc_activate_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_alloc_activate_inbox_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_alloc_global_cache_hit_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_alloc_mmap_activate_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_free_take_elapsed(size_t start_ns, int hit) {
    (void)start_ns; (void)hit;
}
static inline void hz3_s243_residual_on_free_s240_meta_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_free_front_push_elapsed(size_t start_ns,
                                                                int success) {
    (void)start_ns; (void)success;
}
static inline void hz3_s243_residual_on_free_inbox_push_elapsed(size_t start_ns,
                                                                int success) {
    (void)start_ns; (void)success;
}
static inline void hz3_s243_residual_on_free_fallback_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s243_residual_on_free_total_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline size_t hz3_s275_activation_timing_start(void) { return 0; }
static inline void hz3_s275_on_userptr_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_userptr_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_front_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_inbox_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_global_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_meta_mmap_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_mapless_elapsed(size_t start_ns,
                                                             int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s275_on_insert_active_map_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_insert_direct_shadow_elapsed(size_t start_ns) {
    (void)start_ns;
}
static inline void hz3_s275_on_direct_take_elapsed(size_t start_ns, int hit) {
    (void)start_ns;
    (void)hit;
}
static inline void hz3_s280_free_route_on_taken(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_s280_free_route_on_sc_invalid(void) {}
static inline void hz3_s280_free_route_on_sc_oversize(void) {}
static inline void hz3_s280_free_route_on_same_owner(void) {}
static inline void hz3_s280_free_route_on_remote_owner(void) {}
static inline void hz3_s280_free_route_on_unknown_owner(void) {}
static inline void hz3_s280_free_route_on_front_result(int retained, int success) {
    (void)retained;
    (void)success;
}
static inline void hz3_s280_free_route_on_inbox_result(int retained, int success) {
    (void)retained;
    (void)success;
}
static inline void hz3_s280_free_route_on_global_fallback(int had_fast_route,
                                                          int front_reject,
                                                          int inbox_reject) {
    (void)had_fast_route;
    (void)front_reject;
    (void)inbox_reject;
}
static inline void hz3_s280_free_route_on_munmap(void) {}
static inline void hz3_large_aligned_obs_on_normal_alloc(size_t size) {
    (void)size;
}
static inline void hz3_large_aligned_obs_on_invalid(size_t size, size_t alignment) {
    (void)size;
    (void)alignment;
}
static inline void hz3_large_aligned_obs_on_aligned_alloc(size_t size, size_t alignment) {
    (void)size;
    (void)alignment;
}
static inline void hz3_large_aligned_obs_on_aligned_cache_hit(void) {}
static inline void hz3_large_aligned_obs_on_aligned_cache_miss(void) {}
static inline void hz3_large_aligned_obs_on_aligned_mmap(size_t bytes) {
    (void)bytes;
}
static inline void hz3_large_aligned_obs_on_aligned_batch_mmap(size_t bytes) {
    (void)bytes;
}
static inline void hz3_large_aligned_obs_on_free(const Hz3LargeHdr* hdr) {
    (void)hdr;
}
static inline void hz3_large_aligned_obs_on_take_hit(size_t scan_steps) {
    (void)scan_steps;
}
static inline void hz3_large_aligned_obs_on_take_miss(size_t scan_steps) {
    (void)scan_steps;
}
static inline void hz3_large_aligned_obs_on_admit_invalid(void) {}
static inline void hz3_large_aligned_obs_on_admit_oversize(void) {}
static inline void hz3_large_aligned_obs_on_admit_precheck_yes(void) {}
static inline void hz3_large_aligned_obs_on_admit_lock_decision(int can_cache) {
    (void)can_cache;
}
static inline void hz3_large_aligned_obs_on_admit_reuse_skip(void) {}
static inline void hz3_large_aligned_obs_on_admit_final_fit_fail(void) {}
static inline void hz3_large_aligned_obs_on_admit_cache_insert(void) {}
static inline void hz3_large_aligned_obs_on_admit_do_munmap(void) {}
static inline void hz3_large_aligned_obs_on_admit_defer_munmap(void) {}
