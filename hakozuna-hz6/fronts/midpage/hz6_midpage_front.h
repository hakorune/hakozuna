#ifndef HZ6_MIDPAGE_FRONT_H
#define HZ6_MIDPAGE_FRONT_H

#include "../../api/hz6_allocator.h"
#include "../../include/hz6_config.h"
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

int hz6_midpage_prefill_policy_for_class(uint16_t class_id,
                                         Hz6MidPageRunPolicy* policy);

int hz6_midpage_can_allocate(size_t size,
                             size_t align,
                             uint16_t* class_id);

void* hz6_midpage_alloc(Hz6Allocator* allocator,
                        uint16_t class_id,
                        size_t size);

void* hz6_midpage_alloc_with_descriptor(Hz6Allocator* allocator,
                                        uint16_t class_id,
                                        size_t size,
                                        Hz6ObjectDescriptor** out_descriptor);

size_t hz6_midpage_prefill(Hz6Allocator* allocator,
                           uint16_t class_id,
                           size_t count);

int hz6_midpage_free_local(Hz6Allocator* allocator,
                           void* ptr,
                           Hz6RouteResult route);

int hz6_midpage_free_remote(Hz6Allocator* allocator,
                            void* ptr,
                            Hz6RouteResult route);

size_t hz6_midpage_prefill_run(Hz6Allocator* allocator, uint16_t class_id);

const Hz6FrontOps* hz6_midpage_front_ops(void);

#ifdef __cplusplus
}
#endif

#endif
