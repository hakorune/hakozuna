#ifndef HZ6_LARGE128_FRONT_H
#define HZ6_LARGE128_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_LARGE128_CLASS_ID ((uint16_t)8)
#define HZ6_LARGE128_BYTES ((size_t)131072)

const Hz6FrontOps* hz6_large128_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
