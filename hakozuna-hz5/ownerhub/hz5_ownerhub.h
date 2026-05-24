#ifndef HZ5_OWNERHUB_H
#define HZ5_OWNERHUB_H

#include "hz5_internal.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Hz5OwnerHubFront {
  HZ5_OWNERHUB_FRONT_SMALL = 0,
  HZ5_OWNERHUB_FRONT_MID = 1,
  HZ5_OWNERHUB_FRONT_LARGE = 2
} Hz5OwnerHubFront;

#ifndef BENCHLAB_HZ5_LINUX_OWNERHUB_R1
#define BENCHLAB_HZ5_LINUX_OWNERHUB_R1 0
#endif

#ifndef BENCHLAB_HZ5_LINUX_OWNERHUB_R2
#define BENCHLAB_HZ5_LINUX_OWNERHUB_R2 0
#endif

#if defined(__linux__) && \
    (BENCHLAB_HZ5_LINUX_OWNERHUB_R1 || BENCHLAB_HZ5_LINUX_OWNERHUB_R2)
void hz5_ownerhub_mark_pending(Hz5OwnerToken owner,
                               Hz5OwnerHubFront front,
                               uint32_t class_index);
void hz5_ownerhub_clear_pending(Hz5OwnerToken owner,
                                Hz5OwnerHubFront front,
                                uint32_t class_index);
void hz5_ownerhub_note_alloc_miss(Hz5OwnerToken owner,
                                  Hz5OwnerHubFront front,
                                  uint32_t class_index);
void hz5_ownerhub_drain_cross_fronts(Hz5OwnerToken owner,
                                     Hz5OwnerHubFront target_front);
void hz5_ownerhub_stats_print(void);
#else
static inline void hz5_ownerhub_mark_pending(Hz5OwnerToken owner,
                                             Hz5OwnerHubFront front,
                                             uint32_t class_index) {
  (void)owner;
  (void)front;
  (void)class_index;
}

static inline void hz5_ownerhub_clear_pending(Hz5OwnerToken owner,
                                              Hz5OwnerHubFront front,
                                              uint32_t class_index) {
  (void)owner;
  (void)front;
  (void)class_index;
}

static inline void hz5_ownerhub_note_alloc_miss(Hz5OwnerToken owner,
                                                Hz5OwnerHubFront front,
                                                uint32_t class_index) {
  (void)owner;
  (void)front;
  (void)class_index;
}

static inline void hz5_ownerhub_drain_cross_fronts(Hz5OwnerToken owner,
                                                   Hz5OwnerHubFront target) {
  (void)owner;
  (void)target;
}

static inline void hz5_ownerhub_stats_print(void) {}
#endif

#ifdef __cplusplus
}
#endif

#endif
