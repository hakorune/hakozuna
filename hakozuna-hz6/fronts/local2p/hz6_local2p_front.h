#ifndef HZ6_LOCAL2P_FRONT_H
#define HZ6_LOCAL2P_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_LOCAL2P_CLASS_ID ((uint16_t)6)
#define HZ6_LOCAL2P_BYTES ((size_t)65536)

const Hz6FrontOps* hz6_local2p_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
