#include "hz6_midpage_front.h"

const Hz6FrontOps* hz6_midpage_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_MIDPAGE,
      "midpage",
      hz6_midpage_can_allocate,
      hz6_midpage_alloc,
      hz6_midpage_prefill,
      hz6_midpage_free_local,
      hz6_midpage_free_remote,
  };
  return &ops;
}
