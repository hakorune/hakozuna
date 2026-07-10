#ifndef HZ12_SHADOW_H
#define HZ12_SHADOW_H

#include <stdint.h>
#include <stdio.h>

#ifndef HZ12_SHADOW_FLUSH_CAP
#define HZ12_SHADOW_FLUSH_CAP 256u
#endif

#ifndef HZ12_SHADOW_INBOX_CAP
#define HZ12_SHADOW_INBOX_CAP 256u
#endif

#define HZ12_SHADOW_MAX_OWNERS 64u

typedef struct H12ShadowCache {
  void* items[HZ12_SHADOW_FLUSH_CAP];
  uint32_t count;
  uint32_t consumer_id;
} H12ShadowCache;

int h12_shadow_init(uint32_t owner_count);
void h12_shadow_reset(void);
void h12_shadow_on_alloc(void* ptr, uint32_t owner_id);
int h12_shadow_owner_for_ptr(const void* ptr, uint32_t* owner_id);
int h12_shadow_batch_all_owner(void** items, uint32_t count,
                               uint32_t owner_id);
void h12_shadow_cache_init(H12ShadowCache* cache, uint32_t consumer_id);
void h12_shadow_on_free(H12ShadowCache* cache, void* ptr);
void h12_shadow_flush(H12ShadowCache* cache);
void h12_shadow_dump(FILE* out);

#endif /* HZ12_SHADOW_H */
