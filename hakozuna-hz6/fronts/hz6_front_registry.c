#include "hz6_front.h"

#include "large/hz6_large128_front.h"
#include "local2p/hz6_local2p_front.h"
#include "midpage/hz6_midpage_front.h"
#include "toy/hz6_toy_front.h"

const Hz6FrontOps* hz6_front_for_allocation(size_t size,
                                            size_t align,
                                            uint16_t* class_id) {
  if (!class_id || align > 16) {
    return NULL;
  }

  if (size == HZ6_LOCAL2P_BYTES) {
    *class_id = HZ6_LOCAL2P_CLASS_ID;
    return hz6_local2p_front_ops();
  }
  if (size > 4096 && size <= HZ6_MIDPAGE_BYTES) {
    if (hz6_midpage_can_allocate(size, align, class_id)) {
      return hz6_midpage_front_ops();
    }
    return NULL;
  }
  if (size > 4096) {
    if (hz6_large128_can_allocate(size, align, class_id)) {
      return hz6_large128_front_ops();
    }
    return NULL;
  }
  const Hz6FrontOps* toy = hz6_toy_front_ops();
  if (toy->can_allocate(size, align, class_id)) {
    return toy;
  }
  return NULL;
}

const Hz6FrontOps* hz6_front_for_id(uint16_t front_id) {
  switch (front_id) {
    case HZ6_FRONT_LOCAL2P:
      return hz6_local2p_front_ops();
    case HZ6_FRONT_MIDPAGE:
      return hz6_midpage_front_ops();
    case HZ6_FRONT_LARGE:
      return hz6_large128_front_ops();
    case HZ6_FRONT_TOY:
      return hz6_toy_front_ops();
    default:
      return NULL;
  }
}

int hz6_front_remote_rehome_allowed(uint16_t front_id, uint16_t class_id) {
  if (front_id == HZ6_FRONT_LARGE) {
    return hz6_large128_remote_rehome_allowed(class_id);
  }
  return 1;
}
