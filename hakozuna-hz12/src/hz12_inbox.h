#ifndef HZ12_INBOX_H
#define HZ12_INBOX_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_shadow.h"

#ifndef HZ12_INBOX_CAP
#define HZ12_INBOX_CAP 1024u
#endif

typedef struct H12InboxDeferred {
  void* items[HZ12_SHADOW_FLUSH_CAP];
  uint32_t count;
} H12InboxDeferred;

typedef struct H12AdoptionShadow {
  uint32_t retired_owners;
  uint32_t pending_owners;
  uint64_t pending_objects;
} H12AdoptionShadow;

int h12_inbox_init(uint32_t owner_count);
void h12_inbox_deferred_init(H12InboxDeferred* deferred);
void h12_inbox_defer_free(H12InboxDeferred* deferred, void* ptr);
void h12_inbox_flush(H12InboxDeferred* deferred);
uint32_t h12_inbox_drain_owner(uint32_t owner_id);
void h12_inbox_mark_owner_retired(uint32_t owner_id);
H12AdoptionShadow h12_inbox_adoption_shadow_scan(void);
uint32_t h12_inbox_adopt_retired_owner(uint32_t owner_id);
void h12_inbox_dump(FILE* out);

#endif /* HZ12_INBOX_H */
