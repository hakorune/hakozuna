#include "../src/h8_medium_page8k_remote.h"

#include <stdio.h>

#define TURNOVER_ROUNDS 1000u

int main(void) {
  for (uint32_t round = 0; round < TURNOVER_ROUNDS; ++round) {
    void* ptr = h8_page8k_remote_malloc_current(8192u);
    if (!ptr) return 1;
    bool owned = false;
    if (!h8_page8k_remote_free_current(ptr, &owned) || !owned) return 2;
    h8_page8k_remote_thread_shutdown();
  }

  (void)h8_page8k_remote_drain_all_control();
  H8Page8KRemoteStats stats = h8_page8k_remote_stats();
  if (stats.owner_close != TURNOVER_ROUNDS ||
      stats.orphan_adopt != TURNOVER_ROUNDS - 1u ||
      stats.drain_all_owner_visits != 1u || stats.drain_all_limit != 0u ||
      stats.lost_notification != 0u)
    return 3;

  printf("[H8_PAGE8K_THREAD_TURNOVER] rounds=%u close=%llu adopt=%llu "
         "owner_visits=%llu\n",
         TURNOVER_ROUNDS, (unsigned long long)stats.owner_close,
         (unsigned long long)stats.orphan_adopt,
         (unsigned long long)stats.drain_all_owner_visits);
  return 0;
}
