#ifndef HZ12_SPAN_OWNER_SHADOW_H
#define HZ12_SPAN_OWNER_SHADOW_H

#include <stdint.h>

#include "hz12_owner_registry.h"

void h12_span_owner_shadow_reset(void);
int h12_span_owner_shadow_assign(const void* ptr, H12OwnerToken owner);
int h12_span_owner_shadow_clear_if(const void* ptr, H12OwnerToken owner);
int h12_span_owner_shadow_query(uint32_t span_id, H12OwnerToken* out);

#endif /* HZ12_SPAN_OWNER_SHADOW_H */
