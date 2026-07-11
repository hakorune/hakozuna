#include "../include/h8.h"
#include "../src/h8_medium_page_shadow.h"

#include <stdint.h>
#include <stdio.h>

int main(void) {
  void* ptr = h8_malloc(8192u);
  if (!ptr) return 1;

  h8_free((void*)((uintptr_t)ptr + 16u));
  h8_free(ptr);
  h8_free(ptr);

  H8MediumPageShadowStats stats = h8_medium_page_shadow_stats();
  printf("[H8_MEDIUM_PAGE_SHADOW_SMOKE] lookup=%llu hit=%llu miss=%llu "
         "run_mismatch=%llu exact_valid=%llu exact_invalid=%llu "
         "state_match=%llu state_mismatch=%llu\n",
         (unsigned long long)stats.lookup, (unsigned long long)stats.hit,
         (unsigned long long)stats.miss,
         (unsigned long long)stats.run_mismatch,
         (unsigned long long)stats.exact_valid,
         (unsigned long long)stats.exact_invalid,
         (unsigned long long)stats.state_match,
         (unsigned long long)stats.state_mismatch);

  return stats.lookup == 3u && stats.hit == 3u && stats.miss == 0u &&
                 stats.run_mismatch == 0u && stats.exact_valid == 2u &&
                 stats.exact_invalid == 1u && stats.state_match == 2u &&
                 stats.state_mismatch == 0u
             ? 0
             : 2;
}
