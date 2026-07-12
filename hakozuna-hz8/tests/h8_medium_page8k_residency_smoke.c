#include "../src/h8_medium_page8k_remote.h"

#include <stdio.h>

#define RESIDENCY_PAGES 32u
#define RESIDENT_CAP 16u

int main(void) {
  H8Page8KRemotePage* pages[RESIDENCY_PAGES];
  for (uint32_t i = 0; i < RESIDENCY_PAGES; ++i) {
    H8Page8KRemoteOwner* origin =
        h8_page8k_remote_owner_create(3000u + i);
    pages[i] = h8_page8k_remote_page_create(origin);
    if (!origin || !pages[i]) return 1;
    if (!h8_page8k_remote_owner_close(origin)) return 2;
    h8_page8k_remote_owner_destroy(origin);
  }

  H8Page8KRemoteStats before = h8_page8k_remote_stats();
  if (before.page_decommit != RESIDENCY_PAGES - RESIDENT_CAP ||
      before.page_decommit_fail != 0u ||
      before.orphan_resident_empty != RESIDENT_CAP ||
      before.orphan_resident_empty_max != RESIDENT_CAP)
    return 3;

  H8Page8KRemoteOwner* adopter = h8_page8k_remote_owner_create(4000u);
  if (!adopter) return 4;
  for (uint32_t n = 0; n < RESIDENCY_PAGES; ++n) {
    uint32_t index = RESIDENCY_PAGES - 1u - n;
    if (!h8_page8k_remote_adopt_one(adopter)) return 5;
    if (!h8_page8k_remote_alloc(adopter, pages[index])) return 6;
  }

  H8Page8KRemoteStats after = h8_page8k_remote_stats();
  if (after.orphan_adopt != RESIDENCY_PAGES ||
      after.page_recommit != RESIDENCY_PAGES - RESIDENT_CAP ||
      after.orphan_resident_empty != 0u || after.page_decommit_fail != 0u)
    return 7;

  printf("[H8_PAGE8K_RESIDENCY_SMOKE] pages=%u resident_cap=%u "
         "decommit=%llu recommit=%llu resident_final=%llu\n",
         RESIDENCY_PAGES, RESIDENT_CAP,
         (unsigned long long)after.page_decommit,
         (unsigned long long)after.page_recommit,
         (unsigned long long)after.orphan_resident_empty);
  h8_page8k_remote_owner_destroy(adopter);
  return 0;
}
