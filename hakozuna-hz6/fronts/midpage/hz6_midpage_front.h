#ifndef HZ6_MIDPAGE_FRONT_H
#define HZ6_MIDPAGE_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../hz6_front.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HZ6_MIDPAGE_8K_CLASS_ID ((uint16_t)4)
#define HZ6_MIDPAGE_32K_CLASS_ID ((uint16_t)5)
#define HZ6_MIDPAGE_CLASS_ID HZ6_MIDPAGE_32K_CLASS_ID

#define HZ6_MIDPAGE_8K_BYTES ((size_t)8192)
#define HZ6_MIDPAGE_32K_BYTES ((size_t)32768)
#define HZ6_MIDPAGE_BYTES HZ6_MIDPAGE_32K_BYTES
#define HZ6_MIDPAGE_RUN_BYTES ((size_t)65536)

typedef struct Hz6MidPageRunPolicy {
  uint16_t class_id;
  size_t slot_bytes;
  size_t run_bytes;
  size_t slots_per_run;
} Hz6MidPageRunPolicy;

int hz6_midpage_policy_for_size(size_t size,
                                size_t align,
                                Hz6MidPageRunPolicy* policy);

int hz6_midpage_class_bytes(uint16_t class_id, size_t* bytes);

const Hz6FrontOps* hz6_midpage_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
