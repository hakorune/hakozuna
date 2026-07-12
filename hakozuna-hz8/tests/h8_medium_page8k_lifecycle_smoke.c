#include "../src/h8_medium_page8k_remote.h"

#include <stdio.h>

int main(void) {
  H8Page8KRemoteOwner* origin = h8_page8k_remote_owner_create(101u);
  H8Page8KRemoteOwner* remote = h8_page8k_remote_owner_create(102u);
  H8Page8KRemoteOwner* adopter = h8_page8k_remote_owner_create(103u);
  H8Page8KRemotePage* page = h8_page8k_remote_page_create(origin);
  if (!origin || !remote || !adopter || !page) return 1;

  void* slots[8];
  for (size_t i = 0; i < 8u; ++i) {
    slots[i] = h8_page8k_remote_alloc(origin, page);
    if (!slots[i]) return 2;
  }
  if (!h8_page8k_remote_owner_close(origin)) return 3;

  for (size_t i = 0; i < 8u; ++i) {
    bool owned = false;
    if (!h8_page8k_remote_free(remote, slots[i], &owned) || !owned) return 4;
  }
  if (!h8_page8k_remote_adopt_one(adopter)) return 5;
  if (!h8_page8k_remote_quiescent(page)) return 6;

  for (size_t i = 0; i < 8u; ++i) {
    if (!h8_page8k_remote_alloc(adopter, page)) return 7;
  }

  H8Page8KRemoteStats stats = h8_page8k_remote_stats();
  if (stats.owner_close != 1u || stats.orphan_adopt != 1u ||
      stats.remote_claim_success != 8u || stats.drain_slots != 8u ||
      stats.remote_claim_reject != 0u || stats.lost_notification != 0u)
    return 8;

  printf("[H8_PAGE8K_LIFECYCLE_SMOKE] close=%llu adopt=%llu claims=%llu "
         "drained=%llu retry=%llu\n",
         (unsigned long long)stats.owner_close,
         (unsigned long long)stats.orphan_adopt,
         (unsigned long long)stats.remote_claim_success,
         (unsigned long long)stats.drain_slots,
         (unsigned long long)stats.publish_retry);
  h8_page8k_remote_owner_destroy(origin);
  h8_page8k_remote_owner_destroy(remote);
  h8_page8k_remote_owner_destroy(adopter);
  return 0;
}
