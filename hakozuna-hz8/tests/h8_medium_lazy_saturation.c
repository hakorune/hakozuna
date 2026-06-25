#include "../include/h8.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  kThreads = 16,
  kObjectsPerThread = 512,
  kLazyCap = 128u * 1024u * 1024u,
  kNormalCap = 64u * 1024u * 1024u,
  kActiveLiveCap = 20u * 1024u * 1024u,
};

typedef struct SaturationGate {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  int ready;
  int release;
} SaturationGate;

static SaturationGate g_gate = {PTHREAD_MUTEX_INITIALIZER,
                                PTHREAD_COND_INITIALIZER, 0, 0};

static void* saturation_worker(void* arg) {
  (void)arg;
  void** ptrs = calloc(kObjectsPerThread, sizeof(void*));
  if (!ptrs) {
    return (void*)1;
  }

  for (int i = 0; i < kObjectsPerThread; ++i) {
    ptrs[i] = h8_malloc(65536);
    if (!ptrs[i]) {
      free(ptrs);
      return (void*)2;
    }
    ((unsigned char*)ptrs[i])[0] = (unsigned char)i;
    ((unsigned char*)ptrs[i])[65535] = (unsigned char)(i >> 1);
  }
  for (int i = 0; i < kObjectsPerThread; ++i) {
    h8_free(ptrs[i]);
  }
  free(ptrs);

  pthread_mutex_lock(&g_gate.lock);
  g_gate.ready++;
  pthread_cond_broadcast(&g_gate.cond);
  while (!g_gate.release) {
    pthread_cond_wait(&g_gate.cond, &g_gate.lock);
  }
  pthread_mutex_unlock(&g_gate.lock);
  return NULL;
}

static int wait_ready(void) {
  pthread_mutex_lock(&g_gate.lock);
  while (g_gate.ready < kThreads) {
    pthread_cond_wait(&g_gate.cond, &g_gate.lock);
  }
  pthread_mutex_unlock(&g_gate.lock);
  return 1;
}

static void release_workers(void) {
  pthread_mutex_lock(&g_gate.lock);
  g_gate.release = 1;
  pthread_cond_broadcast(&g_gate.cond);
  pthread_mutex_unlock(&g_gate.lock);
}

int main(void) {
  pthread_t tids[kThreads];
  h8_init();

  for (int i = 0; i < kThreads; ++i) {
    if (pthread_create(&tids[i], NULL, saturation_worker, NULL) != 0) {
      perror("pthread_create");
      return 2;
    }
  }

  wait_ready();
  H8DebugStats live = h8_debug_stats();

  if (live.medium_lazy_purge_bytes > kLazyCap ||
      live.medium_lazy_purge_peak > kLazyCap + 128u * 1024u ||
      live.medium_resident_empty_bytes > kNormalCap ||
      live.medium_active_live_empty_bytes > kActiveLiveCap ||
      live.medium_lazy_charge_acquire == 0 ||
      live.medium_lazy_cap_reject == 0 ||
      live.medium_lazy_cas_retry != 0) {
    fprintf(stderr,
            "lazy saturation live gate failed: lazy_bytes=%zu peak=%zu "
            "resident=%zu active=%zu acquire=%zu cap_reject=%zu retry=%zu\n",
            live.medium_lazy_purge_bytes, live.medium_lazy_purge_peak,
            live.medium_resident_empty_bytes,
            live.medium_active_live_empty_bytes,
            live.medium_lazy_charge_acquire, live.medium_lazy_cap_reject,
            live.medium_lazy_cas_retry);
    release_workers();
    for (int i = 0; i < kThreads; ++i) {
      pthread_join(tids[i], NULL);
    }
    return 3;
  }

  release_workers();
  for (int i = 0; i < kThreads; ++i) {
    void* rc = NULL;
    if (pthread_join(tids[i], &rc) != 0 || rc != NULL) {
      fprintf(stderr, "worker failed\n");
      return 4;
    }
  }

  H8DebugStats done = h8_debug_stats();
  if (done.medium_lazy_purge_bytes != 0 ||
      done.medium_resident_empty_bytes != 0 ||
      done.medium_active_live_empty_bytes != 0) {
    fprintf(stderr,
            "lazy saturation drain gate failed: lazy=%zu resident=%zu "
            "active=%zu\n",
            done.medium_lazy_purge_bytes, done.medium_resident_empty_bytes,
            done.medium_active_live_empty_bytes);
    return 5;
  }

  printf("lazy_saturation live_lazy=%zu peak_lazy=%zu resident=%zu active=%zu "
         "acquire=%zu cap_reject=%zu drop=%zu\n",
         live.medium_lazy_purge_bytes, live.medium_lazy_purge_peak,
         live.medium_resident_empty_bytes, live.medium_active_live_empty_bytes,
         live.medium_lazy_charge_acquire, live.medium_lazy_cap_reject,
         done.medium_lazy_charge_drop);
  return 0;
}
