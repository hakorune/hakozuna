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

static void* orphan_source(void* arg) {
  (void)arg;
  void* p = h8_malloc(128);
  if (!p) {
    return (void*)1;
  }
  memset(p, 0x5A, 128);
  return p;
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
  pthread_t orphan_thread;
  if (pthread_create(&orphan_thread, NULL, orphan_source, NULL) != 0) {
    perror("pthread_create");
    return 6;
  }
  void* orphan_ptr = NULL;
  if (pthread_join(orphan_thread, &orphan_ptr) != 0) {
    perror("pthread_join");
    return 7;
  }
  if (orphan_ptr == (void*)1 || !orphan_ptr) {
    fprintf(stderr, "orphan alloc failed\n");
    return 8;
  }
  if (h8_route(orphan_ptr) != H8_ROUTE_VALID) {
    fprintf(stderr, "route invalid after owner exit handoff\n");
    return 9;
  }
  H8Stats before_claim = h8_stats();
  void* adopted = h8_malloc(128);
  if (!adopted) {
    fprintf(stderr, "orphan adoption alloc failed\n");
    return 11;
  }
  H8Stats after_claim = h8_stats();
  if (after_claim.small_span_count != before_claim.small_span_count) {
    fprintf(stderr, "orphan adoption committed a new span\n");
    return 12;
  }
  if (h8_route(adopted) != H8_ROUTE_VALID) {
    fprintf(stderr, "route invalid for adopted alloc\n");
    return 13;
  }
  h8_free(orphan_ptr);
  if (h8_route(orphan_ptr) == H8_ROUTE_VALID) {
    fprintf(stderr, "route still valid after orphan free\n");
    return 14;
  }
  h8_free(adopted);
  if (h8_route(adopted) == H8_ROUTE_VALID) {
    fprintf(stderr, "route still valid after adopted free\n");
    return 15;
  }
  H8Stats stats = h8_stats();
  printf("arena=%zu committed=%zu owners=%zu local=%zu remote=%zu\n",
         stats.arena_reserved_bytes, stats.arena_committed_bytes,
         stats.owner_count, stats.local_alloc_count, stats.remote_publish_count);
  return 0;
}
