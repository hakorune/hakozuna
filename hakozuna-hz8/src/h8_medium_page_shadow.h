#ifndef H8_MEDIUM_PAGE_SHADOW_H
#define H8_MEDIUM_PAGE_SHADOW_H
#include <stdint.h>
typedef struct H8MediumRun H8MediumRun;
typedef struct H8MediumPageShadowStats {
  uint64_t lookup, hit, miss, run_mismatch, exact_valid, exact_invalid;
} H8MediumPageShadowStats;
void h8_medium_page_shadow_register(H8MediumRun* run);
void h8_medium_page_shadow_unregister(H8MediumRun* run);
void h8_medium_page_shadow_compare(const void* ptr, H8MediumRun* run);
H8MediumPageShadowStats h8_medium_page_shadow_stats(void);
#endif
