#include "hz6_large128_front.h"

const Hz6FrontOps* hz6_large128_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_LARGE,
      "largespan",
      hz6_large128_can_allocate,
      hz6_large128_alloc,
      hz6_large128_prefill,
      hz6_large128_free_local,
      hz6_large128_free_remote,
  };
  return &ops;
}
