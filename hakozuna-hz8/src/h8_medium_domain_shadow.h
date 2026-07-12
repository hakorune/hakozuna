#ifndef H8_MEDIUM_DOMAIN_SHADOW_H
#define H8_MEDIUM_DOMAIN_SHADOW_H

#include "h8_medium.h"

#include <stdint.h>

typedef enum H8MediumDomainKind {
  H8_MEDIUM_DOMAIN_NONE = 0,
  H8_MEDIUM_DOMAIN_RUN = 1,
  H8_MEDIUM_DOMAIN_PAGE8K = 2
} H8MediumDomainKind;

typedef struct H8MediumDomainProbe {
  H8MediumDomainKind kind;
  const void* record;
} H8MediumDomainProbe;

typedef struct H8MediumDomainShadowStats {
  uint64_t lookup;
  uint64_t medium_hit;
  uint64_t page8k_hit;
  uint64_t miss;
  uint64_t kind_match;
  uint64_t kind_mismatch;
  uint64_t register_conflict;
  uint64_t medium_register;
  uint64_t medium_unregister;
  uint64_t page8k_register;
} H8MediumDomainShadowStats;

void h8_medium_domain_shadow_register_medium(H8MediumRun* run);
void h8_medium_domain_shadow_unregister_medium(H8MediumRun* run);
void h8_medium_domain_shadow_register_page8k(void* base, const void* record);
H8MediumDomainProbe h8_medium_domain_shadow_lookup(const void* ptr);
void h8_medium_domain_shadow_compare(H8MediumDomainProbe probe,
                                     H8MediumDomainKind expected);
H8MediumDomainShadowStats h8_medium_domain_shadow_stats(void);

#endif
