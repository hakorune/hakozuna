#include "../src/h8_medium_page8k_remote.h"

#include <stdatomic.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define LIFECYCLE_ROUNDS 1000u

typedef struct LifecycleArgs {
  H8Page8KRemoteOwner* remote;
  void* slots[8];
  _Atomic uint32_t publish_round;
  _Atomic uint32_t complete_round;
  _Atomic int failure;
} LifecycleArgs;

static DWORD WINAPI lifecycle_remote_worker(void* opaque) {
  LifecycleArgs* args = (LifecycleArgs*)opaque;
  for (uint32_t round = 1u; round <= LIFECYCLE_ROUNDS; ++round) {
    while (atomic_load_explicit(&args->publish_round, memory_order_acquire) !=
           round)
      SwitchToThread();
    for (size_t i = 0; i < 8u; ++i) {
      bool owned = false;
      if (!h8_page8k_remote_free(args->remote, args->slots[i], &owned) ||
          !owned) {
        atomic_store_explicit(&args->failure, 1, memory_order_release);
        break;
      }
    }
    atomic_store_explicit(&args->complete_round, round, memory_order_release);
  }
  return 0;
}

int main(void) {
  LifecycleArgs args = {0};
  args.remote = h8_page8k_remote_owner_create(201u);
  H8Page8KRemoteOwner* adopter = h8_page8k_remote_owner_create(202u);
  if (!args.remote || !adopter) return 1;
  atomic_init(&args.publish_round, 0u);
  atomic_init(&args.complete_round, 0u);
  atomic_init(&args.failure, 0);

  HANDLE worker =
      CreateThread(NULL, 0u, lifecycle_remote_worker, &args, 0u, NULL);
  if (!worker) return 2;

  for (uint32_t round = 1u; round <= LIFECYCLE_ROUNDS; ++round) {
    H8Page8KRemoteOwner* origin =
        h8_page8k_remote_owner_create(1000u + round);
    H8Page8KRemotePage* page = h8_page8k_remote_page_create(origin);
    if (!origin || !page) return 3;
    for (size_t i = 0; i < 8u; ++i) {
      args.slots[i] = h8_page8k_remote_alloc(origin, page);
      if (!args.slots[i]) return 4;
    }

    atomic_store_explicit(&args.publish_round, round, memory_order_release);
    if (!h8_page8k_remote_owner_close(origin)) return 5;
    while (atomic_load_explicit(&args.complete_round, memory_order_acquire) !=
           round)
      SwitchToThread();
    if (atomic_load_explicit(&args.failure, memory_order_acquire) != 0) return 6;
    if (!h8_page8k_remote_adopt_one(adopter)) return 7;
    if (!h8_page8k_remote_quiescent(page)) return 8;
    for (size_t i = 0; i < 8u; ++i) {
      if (!h8_page8k_remote_alloc(adopter, page)) return 9;
    }
    h8_page8k_remote_owner_destroy(origin);
  }

  WaitForSingleObject(worker, INFINITE);
  CloseHandle(worker);
  H8Page8KRemoteStats stats = h8_page8k_remote_stats();
  const uint64_t expected_slots = (uint64_t)LIFECYCLE_ROUNDS * 8u;
  if (stats.owner_close != LIFECYCLE_ROUNDS ||
      stats.orphan_adopt != LIFECYCLE_ROUNDS ||
      stats.remote_claim_success != expected_slots ||
      stats.drain_slots != expected_slots || stats.remote_claim_reject != 0u ||
      stats.lost_notification != 0u)
    return 10;

  printf("[H8_PAGE8K_LIFECYCLE_STRESS] rounds=%u close=%llu adopt=%llu "
         "claims=%llu drained=%llu retry=%llu\n",
         LIFECYCLE_ROUNDS, (unsigned long long)stats.owner_close,
         (unsigned long long)stats.orphan_adopt,
         (unsigned long long)stats.remote_claim_success,
         (unsigned long long)stats.drain_slots,
         (unsigned long long)stats.publish_retry);
  h8_page8k_remote_owner_destroy(args.remote);
  h8_page8k_remote_owner_destroy(adopter);
  return 0;
}
