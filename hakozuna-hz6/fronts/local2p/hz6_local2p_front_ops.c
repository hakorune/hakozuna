#include "hz6_local2p_front.h"

const Hz6FrontOps* hz6_local2p_front_ops(void) {
  static const Hz6FrontOps ops = {
      HZ6_FRONT_LOCAL2P,
      "local2p",
      hz6_local2p_can_allocate,
      hz6_local2p_alloc,
      hz6_local2p_prefill,
      hz6_local2p_free_local,
      hz6_local2p_free_remote,
  };
  return &ops;
}
