#include "../src/h8_medium_page8k_remote.h"

#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct RemoteArgs {
  H8Page8KRemoteOwner* remote_owner;
  void** slots;
  int rc;
} RemoteArgs;

#if defined(_WIN32)
static DWORD WINAPI remote_worker(void* opaque)
#else
static void* remote_worker(void* opaque)
#endif
{
  RemoteArgs* args = (RemoteArgs*)opaque;
  for (size_t i = 0; i < 8u; ++i) {
    bool owned = false;
    if (!h8_page8k_remote_free(args->remote_owner, args->slots[i], &owned) ||
        !owned) {
      args->rc = 1;
      break;
    }
  }
  bool owned = false;
  if (h8_page8k_remote_free(args->remote_owner, args->slots[0], &owned) ||
      !owned)
    args->rc = 2;
  owned = false;
  if (h8_page8k_remote_free(args->remote_owner,
                            (uint8_t*)args->slots[1] + 16u, &owned) ||
      !owned)
    args->rc = 3;
#if defined(_WIN32)
  return 0;
#else
  return NULL;
#endif
}

int main(void) {
  H8Page8KRemoteOwner* owner = h8_page8k_remote_owner_create(1u);
  H8Page8KRemoteOwner* remote = h8_page8k_remote_owner_create(2u);
  H8Page8KRemotePage* page = h8_page8k_remote_page_create(owner);
  if (!owner || !remote || !page) return 1;

  void* slots[8];
  for (size_t i = 0; i < 8u; ++i) {
    slots[i] = h8_page8k_remote_alloc(owner, page);
    if (!slots[i]) return 2;
  }
  if (h8_page8k_remote_alloc(owner, page)) return 3;

  RemoteArgs args = {remote, slots, 0};
#if defined(_WIN32)
  HANDLE thread = CreateThread(NULL, 0u, remote_worker, &args, 0u, NULL);
  if (!thread) return 4;
  WaitForSingleObject(thread, INFINITE);
  CloseHandle(thread);
#else
  pthread_t thread;
  if (pthread_create(&thread, NULL, remote_worker, &args) != 0) return 4;
  pthread_join(thread, NULL);
#endif
  if (args.rc != 0) return 5 + args.rc;
  if (h8_page8k_remote_drain_all_control() != 0u) return 9;
  if (h8_page8k_remote_drain(owner) != 8u) return 9;
  if (!h8_page8k_remote_quiescent(page)) return 10;

  for (size_t i = 0; i < 8u; ++i) {
    if (!h8_page8k_remote_alloc(owner, page)) return 11;
  }

  H8Page8KRemoteStats stats = h8_page8k_remote_stats();
  if (stats.remote_claim_success != 8u || stats.pending_publish != 8u ||
      stats.queue_notify != 1u || stats.drain_pages != 1u ||
      stats.drain_slots != 8u || stats.duplicate_reject != 1u ||
      stats.interior_reject != 1u || stats.lost_notification != 0u ||
      stats.drain_all_skipped_live < 2u)
    return 12;

  printf("[H8_PAGE8K_REMOTE_SMOKE] claims=%llu notify=%llu drained=%llu "
         "duplicate=%llu interior=%llu quiescent=1\n",
         (unsigned long long)stats.remote_claim_success,
         (unsigned long long)stats.queue_notify,
         (unsigned long long)stats.drain_slots,
         (unsigned long long)stats.duplicate_reject,
         (unsigned long long)stats.interior_reject);
  h8_page8k_remote_owner_destroy(remote);
  return 0;
}
