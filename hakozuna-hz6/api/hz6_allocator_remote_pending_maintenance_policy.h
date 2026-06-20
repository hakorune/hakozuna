#ifndef HZ6_ALLOCATOR_REMOTE_PENDING_MAINTENANCE_POLICY_H
#define HZ6_ALLOCATOR_REMOTE_PENDING_MAINTENANCE_POLICY_H

#include <stdint.h>

#ifndef HZ6_REMOTE_PENDING_TOY_CLASS2_FRONT_MAINTENANCE_GATE_L1
/* Default-off behavior box.  Preserve the useful split-maintenance shape only
 * for Toy class 2, the class used by the 128B cross128 row. */
#define HZ6_REMOTE_PENDING_TOY_CLASS2_FRONT_MAINTENANCE_GATE_L1 0
#endif

#ifndef HZ6_REMOTE_PENDING_TOY_CLASS2_ID
#define HZ6_REMOTE_PENDING_TOY_CLASS2_ID 2u
#endif

static inline int hz6_remote_pending_policy_is_toy_class2(uint16_t front_id,
                                                          uint16_t class_id) {
  return front_id == HZ6_FRONT_TOY &&
         class_id == HZ6_REMOTE_PENDING_TOY_CLASS2_ID;
}

static inline int
hz6_remote_pending_policy_front_external_only(uint16_t front_id,
                                              uint16_t class_id) {
#if HZ6_REMOTE_PENDING_FRONT_MAINTENANCE_EXTERNAL_ONLY_L1
  (void)front_id;
  (void)class_id;
  return 1;
#elif HZ6_REMOTE_PENDING_TOY_CLASS2_FRONT_MAINTENANCE_GATE_L1
  return hz6_remote_pending_policy_is_toy_class2(front_id, class_id);
#else
  (void)front_id;
  (void)class_id;
  return 0;
#endif
}

static inline int hz6_remote_pending_policy_source_gate_allowed(
    uint16_t front_id,
    uint16_t class_id) {
#if HZ6_REMOTE_PENDING_TOY_CLASS2_FRONT_MAINTENANCE_GATE_L1
  return hz6_remote_pending_policy_is_toy_class2(front_id, class_id);
#else
  (void)front_id;
  (void)class_id;
  return 1;
#endif
}

#endif
