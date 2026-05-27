#ifndef HZ6_MIDPAGE_FRONT_H
#define HZ6_MIDPAGE_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_MIDPAGE_CLASS_ID ((uint16_t)5)
#define HZ6_MIDPAGE_BYTES ((size_t)32768)

const Hz6FrontOps* hz6_midpage_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
