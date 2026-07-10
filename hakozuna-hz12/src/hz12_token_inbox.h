#ifndef HZ12_TOKEN_INBOX_H
#define HZ12_TOKEN_INBOX_H

#include <stdint.h>
#include <stdio.h>

#include "hz12_owner_registry.h"

#ifndef HZ12_TOKEN_INBOX_CAP
#define HZ12_TOKEN_INBOX_CAP 1024u
#endif

typedef struct H12TokenInboxStats {
  uint64_t publish_attempt;
  uint64_t publish_accept;
  uint64_t publish_registry_reject;
  uint64_t publish_generation_reject;
  uint64_t publish_overflow;
  uint64_t fallback_objects;
  uint64_t drain_batches;
  uint64_t drain_objects;
  uint64_t drain_generation_reject;
  uint32_t inbox_current_max;
} H12TokenInboxStats;

void h12_token_inbox_reset(void);
int h12_token_inbox_publish(H12OwnerToken token, void** objects,
                            uint32_t count);
uint32_t h12_token_inbox_drain(H12OwnerToken token);
uint32_t h12_token_inbox_pending(H12OwnerToken token);
void h12_token_inbox_stats(H12TokenInboxStats* out);
void h12_token_inbox_dump(FILE* out);

#endif /* HZ12_TOKEN_INBOX_H */
