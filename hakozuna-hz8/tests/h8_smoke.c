#include "../include/h8.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

static void* worker(void* arg) {
  (void)arg;
  void* p = h8_malloc(64);
  if (!p) {
    return (void*)1;
  }
  memset(p, 0xA5, 64);
  h8_free(p);
  return NULL;
}

int main(void) {
  h8_init();
  void* p = h8_malloc(32);
  if (!p) {
    fprintf(stderr, "alloc failed\n");
    return 1;
  }
  if (h8_route(p) != H8_ROUTE_VALID) {
    fprintf(stderr, "route invalid for live alloc\n");
    return 2;
  }
  h8_free(p);
  if (h8_route(p) == H8_ROUTE_VALID) {
    fprintf(stderr, "route still valid after free\n");
    return 3;
  }
  pthread_t t;
  if (pthread_create(&t, NULL, worker, NULL) != 0) {
    perror("pthread_create");
    return 4;
  }
  void* rc = NULL;
  pthread_join(t, &rc);
  if (rc != NULL) {
    return 5;
  }
  H8Stats stats = h8_stats();
  printf("arena=%zu committed=%zu owners=%zu local=%zu remote=%zu\n",
         stats.arena_reserved_bytes, stats.arena_committed_bytes,
         stats.owner_count, stats.local_alloc_count, stats.remote_publish_count);
  return 0;
}
