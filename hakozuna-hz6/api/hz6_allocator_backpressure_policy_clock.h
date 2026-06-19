#ifndef HZ6_ALLOCATOR_BACKPRESSURE_POLICY_CLOCK_H
#define HZ6_ALLOCATOR_BACKPRESSURE_POLICY_CLOCK_H

#include <stdatomic.h>
#include <stdint.h>

static inline int hz6_allocator_backpressure_policy_stride_hit(
    Hz6Allocator* allocator,
    uint32_t stride) {
  if (!allocator) {
    return 0;
  }
  if (stride <= 1u) {
    return 1;
  }
  uint32_t epoch = atomic_fetch_add_explicit(
                       &allocator->remote_backpressure_policy_epoch, 1u,
                       memory_order_relaxed) +
                   1u;
  return epoch % stride == 0u;
}

#endif
