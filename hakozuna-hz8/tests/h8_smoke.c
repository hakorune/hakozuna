#include "../include/h8.h"
#include "../src/h8_medium.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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
  void* p = h8_malloc(256);
  if (!p) {
    return (void*)1;
  }
  memset(p, 0x5A, 256);
  return p;
}

static void* medium_source(void* arg) {
  (void)arg;
  void* p = h8_malloc(9000);
  if (!p) {
    return (void*)1;
  }
  memset(p, 0x4D, 9000);
  return p;
}

static void* adoption_roundtrip(void* arg) {
  size_t size = *(size_t*)arg;
  void* p = h8_malloc(size);
  if (!p) {
    return (void*)1;
  }
  memset(p, 0xC3, size);
  h8_free(p);
  return NULL;
}

static int check_realloc_api(void) {
  char* p = h8_realloc(NULL, 64);
  if (!p) {
    fprintf(stderr, "h8_realloc NULL failed\n");
    return 60;
  }
  for (size_t i = 0; i < 64; ++i) {
    p[i] = (char)(0x40 + (int)(i & 31u));
  }
  char* q = h8_realloc(p, 512);
  if (!q) {
    fprintf(stderr, "h8_realloc small grow failed\n");
    return 61;
  }
  for (size_t i = 0; i < 64; ++i) {
    if (q[i] != (char)(0x40 + (int)(i & 31u))) {
      fprintf(stderr, "h8_realloc small grow lost byte %zu\n", i);
      return 62;
    }
  }
  char* r = h8_realloc(q, 32);
  if (!r) {
    fprintf(stderr, "h8_realloc small shrink failed\n");
    return 63;
  }
  for (size_t i = 0; i < 32; ++i) {
    if (r[i] != (char)(0x40 + (int)(i & 31u))) {
      fprintf(stderr, "h8_realloc small shrink lost byte %zu\n", i);
      return 64;
    }
  }
  h8_free(r);

  char* m = h8_malloc(9000);
  if (!m) {
    fprintf(stderr, "h8_realloc medium setup failed\n");
    return 65;
  }
  memset(m, 0x6D, 9000);
  char* n = h8_realloc(m, 20000);
  if (!n) {
    fprintf(stderr, "h8_realloc medium grow failed\n");
    return 66;
  }
  for (size_t i = 0; i < 9000; ++i) {
    if ((unsigned char)n[i] != 0x6Du) {
      fprintf(stderr, "h8_realloc medium grow lost byte %zu\n", i);
      return 67;
    }
  }
  h8_free(n);

  char* zero = h8_malloc(128);
  if (!zero) {
    fprintf(stderr, "h8_realloc zero setup failed\n");
    return 68;
  }
  if (h8_realloc(zero, 0) != NULL) {
    fprintf(stderr, "h8_realloc zero did not return NULL\n");
    return 69;
  }
  return 0;
}

static int check_medium_scaffold(void) {
  if (h8_medium_size_supported(4096) ||
      !h8_medium_size_supported(4097) ||
      !h8_medium_size_supported(65536) ||
      h8_medium_size_supported(65537)) {
    fprintf(stderr, "medium range classification mismatch\n");
    return 24;
  }
  const uint32_t probes[][3] = {
      {4097u, 0u, 8192u},
      {8192u, 0u, 8192u},
      {8193u, 1u, 16384u},
#if defined(H8_MEDIUM_V12_48K2_CLASS)
      {16385u, 2u, 24576u},
      {24577u, 3u, 32768u},
      {32769u, 4u, 49152u},
      {49153u, 5u, 65536u},
#else
      {16385u, 2u, 32768u},
#if defined(H8_MEDIUM_UPPER48_CLASS)
      {32769u, 3u, 49152u},
      {49153u, 4u, 65536u},
#else
      {32769u, 3u, 65536u},
#endif
#endif
  };
  for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i) {
    uint32_t size = probes[i][0];
    uint32_t class_id = probes[i][1];
    uint32_t rounded = probes[i][2];
    const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
    if (!spec || h8_medium_class_for_size(size) != class_id ||
        h8_medium_rounded_size(size) != rounded ||
        spec->slot_size != rounded ||
        spec->run_size < H8_MEDIUM_RUN_BYTES ||
        spec->slot_count == 0 || spec->bitmap_words != 1u) {
      fprintf(stderr, "medium scaffold spec mismatch\n");
        return 25;
    }
    unsigned char payload[2u * H8_MEDIUM_QUANTUM_BYTES];
    H8MediumRun run;
    memset(&run, 0, sizeof(run));
    run.base = payload;
    run.class_id = (uint16_t)class_id;
    run.slot_size = spec->slot_size;
    run.slot_shift = spec->slot_shift;
    run.slot_count = spec->slot_count;
    run.run_size = spec->run_size;
    size_t slot = 0;
    void* aligned = h8_medium_slot_ptr(&run, 1u < run.slot_count ? 1u : 0u);
    if (!aligned ||
        !h8_medium_slot_index_from_ptr_checked(&run, aligned, &slot) ||
        h8_medium_slot_ptr(&run, slot) != aligned ||
        h8_medium_slot_index_from_ptr_checked(&run, (char*)aligned + 1, NULL) ||
        h8_medium_slot_index_from_ptr_checked(&run,
                                              payload + spec->slot_size *
                                                            spec->slot_count,
                                              NULL)) {
      fprintf(stderr, "medium pointer identity mismatch\n");
      return 27;
    }
  }
  if (h8_medium_class_spec(H8_MEDIUM_CLASS_COUNT) != NULL ||
      h8_medium_rounded_size(4096) != 0u ||
      h8_medium_rounded_size(65537) != 0u) {
    fprintf(stderr, "medium scaffold boundary mismatch\n");
    return 26;
  }
#if defined(H8_MEDIUM_V12_48K2_CLASS)
  H8MediumRun* run24 = h8_medium_run_create_scaffold(2u);
  if (!run24) {
    fprintf(stderr, "medium 24K scaffold run create failed\n");
    return 27;
  }
  void* run24_first = h8_medium_run_alloc_local_scaffold(run24);
  void* run24_second = h8_medium_run_alloc_local_scaffold(run24);
  void* run24_tail = run24->base + ((size_t)run24->slot_size *
                                    (size_t)run24->slot_count);
  if (!run24_first || !run24_second || run24_first == run24_second ||
      !h8_medium_run_free_local_scaffold(run24, run24_first, false) ||
      h8_medium_run_free_local_scaffold(run24, run24_first, false) ||
      h8_medium_run_free_local_scaffold(run24, (char*)run24_second + 1, false) ||
      h8_medium_run_free_local_scaffold(run24, run24_tail, false) ||
      !h8_medium_run_free_local_scaffold(run24, run24_second, false)) {
    fprintf(stderr, "medium 24K local-free decode mismatch\n");
    h8_medium_run_destroy_scaffold(run24);
    return 28;
  }
  h8_medium_run_destroy_scaffold(run24);
  H8MediumRun* run48 = h8_medium_run_create_scaffold(4u);
  if (!run48) {
    fprintf(stderr, "medium 48K scaffold run create failed\n");
    return 29;
  }
  void* run48_first = h8_medium_run_alloc_local_scaffold(run48);
  void* run48_second = h8_medium_run_alloc_local_scaffold(run48);
  void* run48_tail = run48->base + ((size_t)run48->slot_size *
                                    (size_t)run48->slot_count);
  if (!run48_first || !run48_second || run48_first == run48_second ||
      !h8_medium_run_free_local_scaffold(run48, run48_first, false) ||
      h8_medium_run_free_local_scaffold(run48, run48_first, false) ||
      h8_medium_run_free_local_scaffold(run48, (char*)run48_second + 1, false) ||
      h8_medium_run_free_local_scaffold(run48, run48_tail, false) ||
      !h8_medium_run_free_local_scaffold(run48, run48_second, false)) {
    fprintf(stderr, "medium 48K local-free decode mismatch\n");
    h8_medium_run_destroy_scaffold(run48);
    return 30;
  }
  h8_medium_run_destroy_scaffold(run48);
#endif
  H8MediumRun* run = h8_medium_run_create_scaffold(0);
  if (!run) {
    fprintf(stderr, "medium scaffold run create failed\n");
    return 31;
  }
  void* first = h8_medium_run_alloc_local_scaffold(run);
  void* second = h8_medium_run_alloc_local_scaffold(run);
  if (!first || !second || first == second ||
      !h8_medium_run_free_local_scaffold(run, first, false) ||
      h8_medium_run_free_local_scaffold(run, first, false) ||
      h8_medium_run_free_local_scaffold(run, (char*)second + 1, false) ||
      !h8_medium_run_free_local_scaffold(run, second, false)) {
    fprintf(stderr, "medium scaffold local alloc/free mismatch\n");
    h8_medium_run_destroy_scaffold(run);
    return 32;
  }
  h8_medium_run_destroy_scaffold(run);
  return 0;
}

int main(void) {
  const char* mode = getenv("H8_SMOKE_REGULAR_ADOPTION");
  int enable_regular_adoption =
      !(mode && (strcmp(mode, "0") == 0 || strcmp(mode, "false") == 0 ||
                 strcmp(mode, "FALSE") == 0 || strcmp(mode, "off") == 0 ||
                 strcmp(mode, "OFF") == 0));
  if (enable_regular_adoption) {
    setenv("H8_ENABLE_REGULAR_ADOPTION", "1", 1);
  } else {
    unsetenv("H8_ENABLE_REGULAR_ADOPTION");
  }
  int medium_rc = check_medium_scaffold();
  if (medium_rc != 0) {
    return medium_rc;
  }
  int realloc_rc = check_realloc_api();
  if (realloc_rc != 0) {
    return realloc_rc;
  }
  h8_init();
  void* medium = h8_malloc(5000);
  if (!medium) {
    fprintf(stderr, "medium alloc failed\n");
    return 30;
  }
  memset(medium, 0x6D, 5000);
  if (h8_route(medium) != H8_ROUTE_VALID ||
      h8_route((char*)medium + 1) != H8_ROUTE_INVALID) {
    fprintf(stderr, "medium route mismatch\n");
    return 31;
  }
  h8_free(medium);
  if (h8_route(medium) == H8_ROUTE_VALID) {
    fprintf(stderr, "medium route still valid after free\n");
    return 32;
  }
  pthread_t medium_thread;
  if (pthread_create(&medium_thread, NULL, medium_source, NULL) != 0) {
    perror("pthread_create medium");
    return 33;
  }
  void* medium_orphan = NULL;
  if (pthread_join(medium_thread, &medium_orphan) != 0) {
    perror("pthread_join medium");
    return 34;
  }
  if (!medium_orphan || medium_orphan == (void*)1 ||
      h8_route(medium_orphan) != H8_ROUTE_VALID) {
    fprintf(stderr, "medium route invalid after owner exit\n");
    return 35;
  }
  h8_free(medium_orphan);
  if (h8_route(medium_orphan) == H8_ROUTE_VALID) {
    fprintf(stderr, "medium route still valid after cross-owner free\n");
    return 36;
  }
  void* p = h8_malloc(32);
  if (!p) {
    fprintf(stderr, "alloc failed\n");
    return 1;
  }
  if (h8_route(p) != H8_ROUTE_VALID) {
    fprintf(stderr, "route invalid for live alloc\n");
    return 2;
  }
  if (h8_route((char*)p + 1) != H8_ROUTE_INVALID ||
      h8_route((char*)p + 31) != H8_ROUTE_INVALID) {
    fprintf(stderr, "interior pointer was not invalid\n");
    return 23;
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
  H8DebugStats before_claim_dbg = h8_debug_stats();
  void* adopted = h8_malloc(256);
  if (!adopted) {
    fprintf(stderr, "orphan adoption alloc failed\n");
    return 11;
  }
  H8DebugStats after_claim_dbg = h8_debug_stats();
  if (enable_regular_adoption) {
    if (after_claim_dbg.adoption_success_count ==
        before_claim_dbg.adoption_success_count) {
      fprintf(stderr, "regular adoption did not adopt an orphan span\n");
      return 12;
    }
  } else {
    if (after_claim_dbg.adoption_success_count !=
        before_claim_dbg.adoption_success_count) {
      fprintf(stderr, "regular adoption ran while disabled\n");
      return 12;
    }
  }
  if (h8_route(adopted) != H8_ROUTE_VALID) {
    fprintf(stderr, "route invalid for adopted alloc\n");
    return 13;
  }

  if (enable_regular_adoption) {
    const int rounds = 32;
    size_t size = 256;
    H8Stats before_roundtrip = h8_stats();
    H8DebugStats before_dbg = h8_debug_stats();
    for (int i = 0; i < rounds; ++i) {
      pthread_t src;
      if (pthread_create(&src, NULL, orphan_source, NULL) != 0) {
        perror("pthread_create source");
        return 16;
      }
      void* round_orphan = NULL;
      if (pthread_join(src, &round_orphan) != 0) {
        perror("pthread_join source");
        return 17;
      }
      if (!round_orphan || round_orphan == (void*)1) {
        fprintf(stderr, "round source alloc failed\n");
        return 18;
      }

      pthread_t round_adopter;
      if (pthread_create(&round_adopter, NULL, adoption_roundtrip, &size) != 0) {
        perror("pthread_create adopter");
        return 19;
      }
      if (pthread_join(round_adopter, NULL) != 0) {
        perror("pthread_join adopter");
        return 20;
      }
      h8_free(round_orphan);
    }
    H8Stats after_roundtrip = h8_stats();
    H8DebugStats after_dbg = h8_debug_stats();
    if (after_dbg.adoption_success_count <
        before_dbg.adoption_success_count + 1u) {
      fprintf(stderr, "adoption roundtrip did not exercise adoption\n");
      return 21;
    }
    if (after_roundtrip.small_span_count != before_roundtrip.small_span_count + (size_t)rounds) {
      fprintf(stderr, "adoption roundtrip span count mismatch\n");
      return 22;
    }
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
