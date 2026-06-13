// Forward declarations for dispatcher
extern void hz3_small_v2_push_remote_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
#if HZ3_LANE_T16_R90_PAGE_REMOTE
extern void hz3_lane16_push_remote_list_small(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
#endif
#if HZ3_HZ4_BRIDGE
extern void hz3_hz4_bridge_push_remote_list_small(uint8_t owner, int sc,
                                                  void* head, void* tail, uint32_t n);
#endif
extern void hz3_sub4k_push_remote_list(uint8_t dst, int sc, void* head, void* tail, uint32_t n);
extern void hz3_inbox_push_list(uint8_t dst, int sc, void* head, void* tail, uint32_t n);
extern void hz3_inbox_medium_mixed_push_list(uint8_t owner, void* head, void* tail, uint32_t n);
#if HZ3_S300_OVERALIGNED_MEDIUM_RUNS
extern void hz3_inbox_aligned_push_list(uint8_t owner, int sc, void* head, void* tail, uint32_t n);
#endif
