#include "hz6_front.h"

#include "large/hz6_large128_front.h"
#include "local2p/hz6_local2p_front.h"
#include "midpage/hz6_midpage_front.h"
#include "toy/hz6_toy_front.h"

const Hz6FrontOps* hz6_front_for_allocation(size_t size,
                                            size_t align,
                                            uint16_t* class_id) {
  const Hz6FrontOps* fronts[] = {
      hz6_local2p_front_ops(),
      hz6_midpage_front_ops(),
      hz6_large128_front_ops(),
      hz6_toy_front_ops(),
  };
  for (size_t i = 0; i < sizeof(fronts) / sizeof(fronts[0]); ++i) {
    const Hz6FrontOps* front = fronts[i];
    if (front && front->can_allocate &&
        front->can_allocate(size, align, class_id)) {
      return front;
    }
  }
  return NULL;
}

const Hz6FrontOps* hz6_front_for_id(uint16_t front_id) {
  const Hz6FrontOps* fronts[] = {
      hz6_local2p_front_ops(),
      hz6_midpage_front_ops(),
      hz6_large128_front_ops(),
      hz6_toy_front_ops(),
  };
  for (size_t i = 0; i < sizeof(fronts) / sizeof(fronts[0]); ++i) {
    const Hz6FrontOps* front = fronts[i];
    if (front && front->front_id == front_id) {
      return front;
    }
  }
  return NULL;
}
