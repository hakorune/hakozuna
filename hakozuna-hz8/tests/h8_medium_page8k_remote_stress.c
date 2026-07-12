#include "../src/h8_medium_page8k_remote.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

#define STRESS_ROUNDS 10000u
#define STRESS_SLOTS 8u
#define STRESS_PRODUCERS 4u

typedef struct StressArgs {
  H8Page8KRemoteOwner* remote_owner;
  void** slots;
  size_t first_slot;
  size_t slot_count;
  _Atomic uint32_t* publish_round;
  _Atomic uint32_t* complete_count;
  _Atomic int failure;
} StressArgs;

static void stress_yield(void) {
#if defined(_WIN32)
  SwitchToThread();
#else
  sched_yield();
#endif
}

#if defined(_WIN32)
static DWORD WINAPI stress_remote_worker(void* opaque)
#else
static void* stress_remote_worker(void* opaque)
#endif
{
  StressArgs* args = (StressArgs*)opaque;
  for (uint32_t round = 1u; round <= STRESS_ROUNDS; ++round) {
    while (atomic_load_explicit(args->publish_round, memory_order_acquire) !=
           round)
      stress_yield();
    for (size_t n = 0; n < args->slot_count; ++n) {
      size_t slot = args->first_slot + n;
      bool owned = false;
      if (!h8_page8k_remote_free(args->remote_owner, args->slots[slot],
                                  &owned) ||
          !owned) {
        atomic_store_explicit(&args->failure, 1, memory_order_release);
        break;
      }
      stress_yield();
    }
    atomic_fetch_add_explicit(args->complete_count, 1u, memory_order_acq_rel);
  }
#if defined(_WIN32)
  return 0;
#else
  return NULL;
#endif
}

int main(void) {
  H8Page8KRemoteOwner* owner = h8_page8k_remote_owner_create(11u);
  H8Page8KRemotePage* page = h8_page8k_remote_page_create(owner);
  if (!owner || !page) return 1;

  void* slots[STRESS_SLOTS] = {0};
  _Atomic uint32_t publish_round;
  _Atomic uint32_t complete_count;
  atomic_init(&publish_round, 0u);
  atomic_init(&complete_count, 0u);
  StressArgs args[STRESS_PRODUCERS] = {0};

#if defined(_WIN32)
  HANDLE threads[STRESS_PRODUCERS];
#else
  pthread_t threads[STRESS_PRODUCERS];
#endif
  for (size_t i = 0; i < STRESS_PRODUCERS; ++i) {
    args[i].remote_owner = h8_page8k_remote_owner_create((uint32_t)(12u + i));
    args[i].slots = slots;
    args[i].first_slot = i * (STRESS_SLOTS / STRESS_PRODUCERS);
    args[i].slot_count = STRESS_SLOTS / STRESS_PRODUCERS;
    args[i].publish_round = &publish_round;
    args[i].complete_count = &complete_count;
    atomic_init(&args[i].failure, 0);
    if (!args[i].remote_owner) return 2;
#if defined(_WIN32)
    threads[i] =
        CreateThread(NULL, 0u, stress_remote_worker, &args[i], 0u, NULL);
    if (!threads[i]) return 2;
#else
    if (pthread_create(&threads[i], NULL, stress_remote_worker, &args[i]) != 0)
      return 2;
#endif
  }

  uint64_t total_drained = 0u;
  for (uint32_t round = 1u; round <= STRESS_ROUNDS; ++round) {
    for (size_t slot = 0; slot < STRESS_SLOTS; ++slot) {
      slots[slot] = h8_page8k_remote_alloc(owner, page);
      if (!slots[slot]) return 3;
    }
    atomic_store_explicit(&complete_count, 0u, memory_order_release);
    atomic_store_explicit(&publish_round, round, memory_order_release);
    while (atomic_load_explicit(&complete_count, memory_order_acquire) !=
           STRESS_PRODUCERS) {
      total_drained += h8_page8k_remote_drain(owner);
      stress_yield();
    }
    do {
      total_drained += h8_page8k_remote_drain(owner);
    } while (!h8_page8k_remote_quiescent(page));
    for (size_t i = 0; i < STRESS_PRODUCERS; ++i) {
      if (atomic_load_explicit(&args[i].failure, memory_order_acquire) != 0)
        return 4;
    }
  }

#if defined(_WIN32)
  WaitForMultipleObjects(STRESS_PRODUCERS, threads, TRUE, INFINITE);
  for (size_t i = 0; i < STRESS_PRODUCERS; ++i) CloseHandle(threads[i]);
#else
  for (size_t i = 0; i < STRESS_PRODUCERS; ++i)
    pthread_join(threads[i], NULL);
#endif

  H8Page8KRemoteStats stats = h8_page8k_remote_stats();
  const uint64_t expected = (uint64_t)STRESS_ROUNDS * STRESS_SLOTS;
  if (stats.remote_claim_attempt != expected ||
      stats.remote_claim_success != expected ||
      stats.remote_claim_reject != 0u || stats.pending_publish != expected ||
      stats.drain_slots != expected || total_drained != expected ||
      stats.duplicate_reject != 0u || stats.interior_reject != 0u ||
      stats.lost_notification != 0u || !h8_page8k_remote_quiescent(page))
    return 5;

  printf("[H8_PAGE8K_REMOTE_STRESS] producers=%u rounds=%u claims=%llu notify=%llu "
         "dirty=%llu drain_pages=%llu drained=%llu depth_max=%llu\n",
         STRESS_PRODUCERS, STRESS_ROUNDS,
         (unsigned long long)stats.remote_claim_success,
         (unsigned long long)stats.queue_notify,
         (unsigned long long)stats.queue_dirty,
         (unsigned long long)stats.drain_pages,
         (unsigned long long)stats.drain_slots,
         (unsigned long long)stats.inbox_depth_max);
  for (size_t i = 0; i < STRESS_PRODUCERS; ++i)
    h8_page8k_remote_owner_destroy(args[i].remote_owner);
  return 0;
}
