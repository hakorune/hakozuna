#include "../include/h8.h"
#include "../src/h8_adaptive_shadow.h"
#include "../src/h8_medium.h"
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
#include "../src/h8_medium_domain_shadow.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef void* h8_smoke_thread_ret_t;
typedef struct H8SmokeThreadStart {
  h8_smoke_thread_ret_t (*fn)(void*);
  void* arg;
  void* result;
} H8SmokeThreadStart;
typedef struct h8_smoke_thread_t {
  HANDLE handle;
  H8SmokeThreadStart* start;
} h8_smoke_thread_t;
#define H8_SMOKE_THREAD_RETVAL(code) ((void*)(uintptr_t)(code))
#define H8_SMOKE_THREAD_RETVAL_PTR(ptr) (ptr)
static DWORD WINAPI h8_smoke_thread_trampoline(void* arg) {
  H8SmokeThreadStart* start = (H8SmokeThreadStart*)arg;
  start->result = start->fn(start->arg);
  return 0;
}
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  H8SmokeThreadStart* start =
      (H8SmokeThreadStart*)calloc(1, sizeof(*start));
  if (!start) {
    return -1;
  }
  start->fn = fn;
  start->arg = arg;
  thread->start = start;
  thread->handle =
      CreateThread(NULL, 0, h8_smoke_thread_trampoline, start, 0, NULL);
  if (!thread->handle) {
    free(start);
    thread->start = NULL;
    return -1;
  }
  return 0;
}
static int h8_smoke_thread_join(h8_smoke_thread_t thread, void** out) {
  if (WaitForSingleObject(thread.handle, INFINITE) != WAIT_OBJECT_0) {
    if (thread.handle) {
      CloseHandle(thread.handle);
    }
    free(thread.start);
    return -1;
  }
  if (out) {
    *out = thread.start ? thread.start->result : NULL;
  }
  if (thread.handle) {
    CloseHandle(thread.handle);
  }
  free(thread.start);
  return 0;
}
static void h8_smoke_set_regular_adoption(int enabled) {
  if (enabled) {
    _putenv("H8_ENABLE_REGULAR_ADOPTION=1");
  } else {
    _putenv("H8_ENABLE_REGULAR_ADOPTION=");
  }
}
#else
#include <pthread.h>
typedef pthread_t h8_smoke_thread_t;
typedef void* h8_smoke_thread_ret_t;
#define H8_SMOKE_THREAD_RETVAL(code) ((void*)(uintptr_t)(code))
#define H8_SMOKE_THREAD_RETVAL_PTR(ptr) (ptr)
static int h8_smoke_thread_create(h8_smoke_thread_t* thread,
                                  h8_smoke_thread_ret_t (*fn)(void*),
                                  void* arg) {
  return pthread_create(thread, NULL, fn, arg);
}
static int h8_smoke_thread_join(h8_smoke_thread_t thread, void** out) {
  return pthread_join(thread, out);
}
static void h8_smoke_set_regular_adoption(int enabled) {
  if (enabled) {
    setenv("H8_ENABLE_REGULAR_ADOPTION", "1", 1);
  } else {
    unsetenv("H8_ENABLE_REGULAR_ADOPTION");
  }
}
#endif

#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
typedef struct H8StableConcurrentCtx {
  H8MediumRun run;
  _Atomic uint32_t slot_state[4];
  _Atomic uint64_t pending_bits[1];
  _Atomic int stop;
  _Atomic uint64_t observations;
} H8StableConcurrentCtx;

typedef struct H8StablePendingCtx {
  _Atomic(void*) ptr;
  _Atomic int ready;
  _Atomic int release;
} H8StablePendingCtx;

static h8_smoke_thread_ret_t h8_stable_pending_owner(void* arg) {
  H8StablePendingCtx* ctx = (H8StablePendingCtx*)arg;
  void* ptr = h8_malloc(16384u);
  if (!ptr) {
    atomic_store_explicit(&ctx->ready, 1, memory_order_release);
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  atomic_store_explicit(&ctx->ptr, ptr, memory_order_release);
  atomic_store_explicit(&ctx->ready, 1, memory_order_release);
  while (!atomic_load_explicit(&ctx->release, memory_order_acquire)) {
    atomic_signal_fence(memory_order_seq_cst);
  }
  void* trigger = h8_malloc(16384u);
  if (trigger) {
    h8_free(trigger);
  }
  return H8_SMOKE_THREAD_RETVAL(0);
}

static bool h8_stable_pending_live_owner_roundtrip(void) {
  H8StablePendingCtx ctx = {0};
  atomic_init(&ctx.ptr, NULL);
  atomic_init(&ctx.ready, 0);
  atomic_init(&ctx.release, 0);
  h8_smoke_thread_t owner;
  if (h8_smoke_thread_create(&owner, h8_stable_pending_owner, &ctx) != 0) {
    return false;
  }
  while (!atomic_load_explicit(&ctx.ready, memory_order_acquire)) {
    atomic_signal_fence(memory_order_seq_cst);
  }
  void* ptr = atomic_load_explicit(&ctx.ptr, memory_order_acquire);
  h8_free(ptr);
  atomic_store_explicit(&ctx.release, 1, memory_order_release);
  void* result = NULL;
  return h8_smoke_thread_join(owner, &result) == 0 &&
         (uintptr_t)result == 0u;
}

static h8_smoke_thread_ret_t h8_stable_record_reader(void* arg) {
  H8StableConcurrentCtx* ctx = (H8StableConcurrentCtx*)arg;
  while (!atomic_load_explicit(&ctx->stop, memory_order_acquire)) {
    H8MediumDomainProbe probe =
        h8_medium_domain_shadow_lookup(ctx->run.base);
    if (probe.kind == H8_MEDIUM_DOMAIN_RUN) {
      h8_medium_domain_stable_compare(probe, &ctx->run);
      atomic_fetch_add_explicit(&ctx->observations, 1u, memory_order_relaxed);
    }
  }
  return H8_SMOKE_THREAD_RETVAL(0);
}

static bool h8_stable_record_concurrent_turnover(void) {
  H8StableConcurrentCtx ctx = {0};
  ctx.run.base = (uint8_t*)(uintptr_t)UINT64_C(0x200000000);
  ctx.run.class_id = 1u;
  ctx.run.slot_count = 4u;
  ctx.run.slot_size = 16384u;
  ctx.run.slot_shift = 14u;
  ctx.run.run_size = H8_MEDIUM_QUANTUM_BYTES;
  ctx.run.slot_state = ctx.slot_state;
  ctx.run.pending_bits = ctx.pending_bits;
  atomic_init(&ctx.stop, 0);
  atomic_init(&ctx.observations, 0u);
  h8_smoke_thread_t reader;
  if (h8_smoke_thread_create(&reader, h8_stable_record_reader, &ctx) != 0) {
    return false;
  }
  for (size_t i = 0u; i < 10000u; ++i) {
    h8_medium_domain_shadow_register_medium(&ctx.run);
    for (size_t spin = 0u; spin < 32u; ++spin) {
      atomic_signal_fence(memory_order_seq_cst);
    }
    h8_medium_domain_shadow_unregister_medium(&ctx.run);
  }
  atomic_store_explicit(&ctx.stop, 1, memory_order_release);
  if (h8_smoke_thread_join(reader, NULL) != 0) {
    return false;
  }
  return atomic_load_explicit(&ctx.observations, memory_order_relaxed) != 0u;
}
#endif

static h8_smoke_thread_ret_t worker(void* arg) {
  (void)arg;
  void* p = h8_malloc(64);
  if (!p) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  memset(p, 0xA5, 64);
  h8_free(p);
  return H8_SMOKE_THREAD_RETVAL(0);
}

static h8_smoke_thread_ret_t orphan_source(void* arg) {
  (void)arg;
  void* p = h8_malloc(256);
  if (!p) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  memset(p, 0x5A, 256);
  return H8_SMOKE_THREAD_RETVAL_PTR(p);
}

static h8_smoke_thread_ret_t medium_source(void* arg) {
  (void)arg;
  void* p = h8_malloc(9000);
  if (!p) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  memset(p, 0x4D, 9000);
  return H8_SMOKE_THREAD_RETVAL_PTR(p);
}

static h8_smoke_thread_ret_t adoption_roundtrip(void* arg) {
  size_t size = *(size_t*)arg;
  void* p = h8_malloc(size);
  if (!p) {
    return H8_SMOKE_THREAD_RETVAL(1);
  }
  memset(p, 0xC3, size);
  h8_free(p);
  return H8_SMOKE_THREAD_RETVAL(0);
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
#if defined(_WIN32)
  int enable_regular_adoption =
      mode && (strcmp(mode, "1") == 0 || strcmp(mode, "true") == 0 ||
               strcmp(mode, "TRUE") == 0 || strcmp(mode, "on") == 0 ||
               strcmp(mode, "ON") == 0 || strcmp(mode, "yes") == 0 ||
               strcmp(mode, "YES") == 0);
#else
  int enable_regular_adoption =
      !(mode && (strcmp(mode, "0") == 0 || strcmp(mode, "false") == 0 ||
                 strcmp(mode, "FALSE") == 0 || strcmp(mode, "off") == 0 ||
                 strcmp(mode, "OFF") == 0));
#endif
  h8_smoke_set_regular_adoption(enable_regular_adoption);
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
  h8_smoke_thread_t medium_thread;
  if (h8_smoke_thread_create(&medium_thread, medium_source, NULL) != 0) {
    perror("thread_create medium");
    return 33;
  }
  void* medium_orphan = NULL;
  if (h8_smoke_thread_join(medium_thread, &medium_orphan) != 0) {
    perror("thread_join medium");
    return 34;
  }
  H8RouteKind medium_orphan_route =
      medium_orphan ? h8_route(medium_orphan) : H8_ROUTE_INVALID;
  if (!medium_orphan || medium_orphan == (void*)1 ||
      medium_orphan_route != H8_ROUTE_VALID) {
    H8MediumRun* medium_run =
        medium_orphan ? h8_medium_directory_find(medium_orphan) : NULL;
    fprintf(stderr,
            "medium route invalid after owner exit ptr=%p route=%d run=%p\n",
            medium_orphan, (int)medium_orphan_route, (void*)medium_run);
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
  h8_smoke_thread_t t;
  if (h8_smoke_thread_create(&t, worker, NULL) != 0) {
    perror("thread_create");
    return 4;
  }
  void* rc = NULL;
  h8_smoke_thread_join(t, &rc);
  if (rc != NULL) {
    return 5;
  }
  h8_smoke_thread_t orphan_thread;
  if (h8_smoke_thread_create(&orphan_thread, orphan_source, NULL) != 0) {
    perror("thread_create");
    return 6;
  }
  void* orphan_ptr = NULL;
  if (h8_smoke_thread_join(orphan_thread, &orphan_ptr) != 0) {
    perror("thread_join");
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
      h8_smoke_thread_t src;
      if (h8_smoke_thread_create(&src, orphan_source, NULL) != 0) {
        perror("thread_create source");
        return 16;
      }
      void* round_orphan = NULL;
      if (h8_smoke_thread_join(src, &round_orphan) != 0) {
        perror("thread_join source");
        return 17;
      }
      if (!round_orphan || round_orphan == (void*)1) {
        fprintf(stderr, "round source alloc failed\n");
        return 18;
      }

      h8_smoke_thread_t round_adopter;
      if (h8_smoke_thread_create(&round_adopter, adoption_roundtrip, &size) != 0) {
        perror("thread_create adopter");
        return 19;
      }
      if (h8_smoke_thread_join(round_adopter, NULL) != 0) {
        perror("thread_join adopter");
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
#if defined(H8_LARGE_DIRECT_OWNED_L1)
  void* large_direct = h8_malloc(96u * 1024u);
  if (!large_direct) {
    fprintf(stderr, "large direct alloc failed\n");
    return 23;
  }
  if (h8_route(large_direct) != H8_ROUTE_VALID) {
    fprintf(stderr, "large direct exact route is not valid\n");
    return 24;
  }
  if (h8_route((uint8_t*)large_direct + 16u) != H8_ROUTE_INVALID) {
    fprintf(stderr, "large direct interior route is not invalid\n");
    return 25;
  }
  void* grown_direct = h8_realloc(large_direct, 112u * 1024u);
  if (!grown_direct || h8_route(grown_direct) != H8_ROUTE_VALID) {
    fprintf(stderr, "large direct realloc failed\n");
    return 26;
  }
  h8_free(grown_direct);
  if (h8_route(grown_direct) == H8_ROUTE_VALID) {
    fprintf(stderr, "large direct route still valid after free\n");
    return 27;
  }
#endif
#if defined(H8_UNIFIED_MEDIUM_DOMAIN_STABLE_RECORD_L0)
  if (!h8_medium_domain_stable_turnover_test(10000u)) {
    fprintf(stderr, "stable record turnover test failed\n");
    return 29;
  }
  if (!h8_stable_record_concurrent_turnover()) {
    fprintf(stderr, "stable record concurrent turnover test failed\n");
    return 30;
  }
  if (!h8_stable_pending_live_owner_roundtrip()) {
    fprintf(stderr, "stable pending live-owner roundtrip failed\n");
    return 31;
  }
  H8MediumDomainShadowStats domain = h8_medium_domain_shadow_stats();
  if (domain.stable_record_alloc == 0u || domain.stable_live_lookup == 0u ||
      domain.stable_record_pool_fallback != 0u ||
      domain.stable_backend_mismatch != 0u ||
      domain.stable_geometry_mismatch != 0u ||
      domain.stable_unregister_missing != 0u ||
      domain.stable_turnover_pass != 1u ||
      domain.stable_turnover_fail != 0u ||
      domain.stable_record_reuse_detected != 0u ||
      domain.stable_tombstone_lookup < 10000u ||
      domain.stable_exact_valid < 20000u ||
      domain.stable_exact_invalid < 10000u ||
      domain.stable_exact_tombstone < 10000u ||
      domain.stable_exact_mismatch != 0u ||
      domain.stable_slot_init == 0u || domain.stable_slot_sync == 0u ||
      domain.stable_slot_mismatch != 0u ||
      domain.stable_slot_bound_fallback != 0u ||
      domain.stable_slot_final_sync == 0u ||
      domain.stable_slot_sync_after_closing != 0u ||
      domain.stable_pending_init == 0u ||
      domain.stable_pending_sync == 0u ||
      domain.stable_pending_final_sync == 0u ||
      domain.stable_pending_mismatch != 0u ||
      domain.stable_pending_sync_after_closing != 0u ||
      domain.stable_lock_acquire == 0u ||
      domain.stable_unlock_mismatch != 0u ||
      domain.stable_unregister_lock == 0u ||
      domain.high_address_fallback == 0u) {
    fprintf(stderr,
            "stable record smoke failed alloc=%llu live=%llu fallback=%llu "
            "backend_mismatch=%llu geometry_mismatch=%llu "
            "unregister_missing=%llu turnover_pass=%llu "
            "turnover_fail=%llu reuse=%llu tombstone=%llu "
            "exact_valid=%llu exact_invalid=%llu exact_tombstone=%llu "
            "exact_mismatch=%llu slot_init=%llu slot_sync=%llu "
            "slot_mismatch=%llu slot_bound=%llu slot_final=%llu "
            "slot_after_closing=%llu mirror_alloc_auth_free=%llu "
            "mirror_free_auth_alloc=%llu slot_other=%llu "
            "slot_without_record=%llu pending_init=%llu pending_sync=%llu "
            "pending_final=%llu pending_mismatch=%llu "
            "pending_after_closing=%llu pending_without_record=%llu "
            "lock_acquire=%llu lock_fallback=%llu unlock_mismatch=%llu "
            "unregister_lock=%llu\n",
            (unsigned long long)domain.stable_record_alloc,
            (unsigned long long)domain.stable_live_lookup,
            (unsigned long long)domain.stable_record_pool_fallback,
            (unsigned long long)domain.stable_backend_mismatch,
            (unsigned long long)domain.stable_geometry_mismatch,
            (unsigned long long)domain.stable_unregister_missing,
            (unsigned long long)domain.stable_turnover_pass,
            (unsigned long long)domain.stable_turnover_fail,
            (unsigned long long)domain.stable_record_reuse_detected,
            (unsigned long long)domain.stable_tombstone_lookup,
            (unsigned long long)domain.stable_exact_valid,
            (unsigned long long)domain.stable_exact_invalid,
            (unsigned long long)domain.stable_exact_tombstone,
            (unsigned long long)domain.stable_exact_mismatch,
            (unsigned long long)domain.stable_slot_init,
            (unsigned long long)domain.stable_slot_sync,
            (unsigned long long)domain.stable_slot_mismatch,
            (unsigned long long)domain.stable_slot_bound_fallback,
            (unsigned long long)domain.stable_slot_final_sync,
            (unsigned long long)domain.stable_slot_sync_after_closing,
            (unsigned long long)
                domain.stable_slot_mirror_alloc_authority_free,
            (unsigned long long)
                domain.stable_slot_mirror_free_authority_alloc,
            (unsigned long long)domain.stable_slot_other_mismatch,
            (unsigned long long)domain.stable_slot_note_without_record,
            (unsigned long long)domain.stable_pending_init,
            (unsigned long long)domain.stable_pending_sync,
            (unsigned long long)domain.stable_pending_final_sync,
            (unsigned long long)domain.stable_pending_mismatch,
            (unsigned long long)domain.stable_pending_sync_after_closing,
            (unsigned long long)domain.stable_pending_note_without_record,
            (unsigned long long)domain.stable_lock_acquire,
            (unsigned long long)domain.stable_lock_fallback,
            (unsigned long long)domain.stable_unlock_mismatch,
            (unsigned long long)domain.stable_unregister_lock);
    return 32;
  }
  printf("stable_domain records=%llu live=%llu tombstone=%llu "
         "slot_sync=%llu slot_final=%llu pending_sync=%llu "
         "pending_final=%llu stable_lock=%llu lock_fallback=%llu "
         "unregister_lock=%llu pool_bytes=%llu\n",
         (unsigned long long)domain.stable_record_alloc,
         (unsigned long long)domain.stable_live_lookup,
         (unsigned long long)domain.stable_tombstone_lookup,
         (unsigned long long)domain.stable_slot_sync,
         (unsigned long long)domain.stable_slot_final_sync,
         (unsigned long long)domain.stable_pending_sync,
         (unsigned long long)domain.stable_pending_final_sync,
         (unsigned long long)domain.stable_lock_acquire,
         (unsigned long long)domain.stable_lock_fallback,
         (unsigned long long)domain.stable_unregister_lock,
         (unsigned long long)domain.stable_pool_current_bytes);
#endif
  H8Stats stats = h8_stats();
#if defined(H8_ADAPTIVE_TRANSFER_SHADOW_L0)
  H8AdaptiveShadowSnapshot adaptive = h8_adaptive_shadow_snapshot();
  uint64_t small_refill = 0u;
  uint64_t medium_source = 0u;
  for (uint32_t i = 0u; i < H8_CLASS_COUNT; ++i) {
    small_refill += adaptive.small_refill[i];
  }
  for (uint32_t i = 0u; i < H8_MEDIUM_CLASS_COUNT; ++i) {
    medium_source += adaptive.medium_source[i];
  }
  if (small_refill == 0u || medium_source == 0u ||
      adaptive.recommendation[H8_ADAPTIVE_SHADOW_TRANSFER_PRESSURE] == 0u) {
    fprintf(stderr,
            "adaptive shadow did not observe expected slow-path pressure\n");
    return 28;
  }
  h8_adaptive_shadow_dump();
#endif
  printf("arena=%zu committed=%zu owners=%zu local=%zu remote=%zu\n",
         stats.arena_reserved_bytes, stats.arena_committed_bytes,
         stats.owner_count, stats.local_alloc_count, stats.remote_publish_count);
  return 0;
}
