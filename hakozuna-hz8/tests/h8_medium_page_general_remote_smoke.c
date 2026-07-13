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
  size_t count;
  int rc;
} RemoteArgs;

#if defined(_WIN32)
static DWORD WINAPI remote_worker(void* opaque)
#else
static void* remote_worker(void* opaque)
#endif
{
  RemoteArgs* args = (RemoteArgs*)opaque;
  for (size_t i = 0; i < args->count; ++i) {
    bool owned = false;
    if (!h8_page8k_remote_free(args->remote_owner, args->slots[i], &owned) ||
        !owned) {
      args->rc = 1;
      break;
    }
  }
#if defined(_WIN32)
  return 0;
#else
  return NULL;
#endif
}

static int run_case(size_t size, uint32_t token) {
  const size_t count = 65536u / size;
  H8Page8KRemoteOwner* owner = h8_page8k_remote_owner_create(token);
  H8Page8KRemoteOwner* remote = h8_page8k_remote_owner_create(token + 1u);
  H8Page8KRemotePage* page = h8_page8k_remote_page_create_size(owner, size);
  if (!owner || !remote || !page) return 1;

  void* slots[8] = {0};
  for (size_t i = 0; i < count; ++i) {
    slots[i] = h8_page8k_remote_alloc(owner, page);
    if (!slots[i]) return 2;
  }
  if (h8_page8k_remote_alloc(owner, page)) return 3;

  RemoteArgs args = {remote, slots, count, 0};
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
  if (args.rc != 0 || h8_page8k_remote_drain(owner) != count ||
      !h8_page8k_remote_quiescent(page))
    return 5;

  for (size_t i = 0; i < count; ++i) {
    if (!h8_page8k_remote_alloc(owner, page)) return 6;
  }
  h8_page8k_remote_owner_destroy(remote);
  h8_page8k_remote_owner_destroy(owner);
  return 0;
}

int main(void) {
  if (run_case(8192u, 10u) != 0 || run_case(16384u, 20u) != 0 ||
      run_case(32768u, 30u) != 0)
    return 1;
  H8Page8KRemoteStats stats = h8_page8k_remote_stats();
  if (stats.remote_claim_success != 14u || stats.drain_slots != 14u ||
      stats.remote_claim_reject != 0u || stats.lost_notification != 0u)
    return 2;
  printf("[H8_PAGE_GENERAL_REMOTE_SMOKE] claims=%llu drained=%llu "
         "reject=%llu lost=%llu\n",
         (unsigned long long)stats.remote_claim_success,
         (unsigned long long)stats.drain_slots,
         (unsigned long long)stats.remote_claim_reject,
         (unsigned long long)stats.lost_notification);
  return 0;
}
