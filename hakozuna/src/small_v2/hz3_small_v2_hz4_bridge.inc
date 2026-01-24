// hz3_small_v2_hz4_bridge.inc - HZ4 bridge box (hz3 bench-compatible)
// Box Theory: Boundary-only (remote dispatch + collect). No hot-path changes.

#if HZ3_HZ4_BRIDGE

void hz3_hz4_bridge_push_remote_list_small(uint8_t owner, int sc,
                                           void* head, void* tail, uint32_t n) {
    hz3_lane16_push_remote_list_small(owner, sc, head, tail, n);
}

uint32_t hz3_hz4_bridge_pop_batch_small(uint8_t owner, int sc,
                                        void** out, uint32_t want) {
    return hz3_lane16_pop_batch_small(owner, sc, out, want);
}

#endif
