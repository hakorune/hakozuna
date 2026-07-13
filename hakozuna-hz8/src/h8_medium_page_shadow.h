#ifndef H8_MEDIUM_PAGE_SHADOW_H
#define H8_MEDIUM_PAGE_SHADOW_H
#include <stddef.h>
#include <stdint.h>
typedef struct H8MediumRun H8MediumRun;
typedef struct H8MediumPageShadowStats {
  uint64_t lookup, hit, miss, run_mismatch, exact_valid, exact_invalid;
  uint64_t state_match, state_mismatch;
  uint64_t geometry_match, geometry_mismatch;
} H8MediumPageShadowStats;
void h8_medium_page_shadow_register(H8MediumRun* run);
void h8_medium_page_shadow_unregister(H8MediumRun* run);
void h8_medium_page_shadow_compare(const void* ptr, H8MediumRun* run);
void h8_medium_page_shadow_note_alloc(H8MediumRun* run, size_t slot);
void h8_medium_page_shadow_note_free(H8MediumRun* run, size_t slot);
H8MediumPageShadowStats h8_medium_page_shadow_stats(void);
#endif
