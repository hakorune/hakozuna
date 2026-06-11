#include "../hz7.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect(int cond, const char* label) {
  if (!cond) {
    fprintf(stderr, "hz7 smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

static int expect_route_transition(void* ptr,
                                   H7RouteKind live_kind,
                                   H7RouteKind dead_kind,
                                   const char* live_label,
                                   const char* dead_label) {
  if (!expect(h7_route(ptr) == live_kind, live_label)) {
    return 0;
  }
  h7_free(ptr);
  if (!expect(h7_route(ptr) == dead_kind, dead_label)) {
    return 0;
  }
  return 1;
}

int main(void) {
  void* a = h7_malloc(64);
  void* b = h7_malloc(4096);
  void* c = h7_malloc(8192);
  void* d = h7_malloc(16384);
  void* e = h7_malloc(32768);
  void* f = h7_malloc(65536);
  void* e2;
  void* f2;
  unsigned char* z = (unsigned char*)h7_calloc(32, 8);
  H7Stats stats;
  size_t i;

  if (!expect(a != 0, "malloc 64") || !expect(b != 0, "malloc 4096") ||
      !expect(c != 0, "malloc 8192") || !expect(d != 0, "malloc 16384") ||
      !expect(e != 0, "malloc 32768 direct") ||
      !expect(f != 0, "malloc 65536 direct") ||
      !expect(z != 0, "calloc")) {
    return 1;
  }

  if (!expect_route_transition(a,
                               H7_ROUTE_VALID,
                               H7_ROUTE_INVALID,
                               "route valid small",
                               "route invalid freed small") ||
      !expect(h7_route((unsigned char*)a + 1) == H7_ROUTE_INVALID,
              "route invalid small interior") ||
      !expect_route_transition(c,
                               H7_ROUTE_VALID,
                               H7_ROUTE_INVALID,
                               "route valid medium 8K",
                               "route invalid freed medium 8K") ||
      !expect(h7_route((unsigned char*)c + 1) == H7_ROUTE_INVALID,
              "route invalid medium interior") ||
      !expect_route_transition(e,
                               H7_ROUTE_VALID,
                               H7_ROUTE_INVALID,
                               "route valid 32K direct",
                               "route invalid freed 32K direct") ||
      !expect_route_transition(f,
                               H7_ROUTE_VALID,
                               H7_ROUTE_INVALID,
                               "route valid 64K direct",
                               "route invalid freed 64K direct") ||
      !expect(h7_route((unsigned char*)f + 1) == H7_ROUTE_INVALID,
              "route invalid direct interior") ||
      !expect(h7_route(&stats) == H7_ROUTE_MISS, "route miss foreign")) {
    return 1;
  }

  for (i = 0; i < 32u * 8u; ++i) {
    if (!expect(z[i] == 0, "calloc zero fill")) {
      return 1;
    }
  }

  memset(b, 0x5A, 4096);
  memset(d, 0xD4, 16384);

  stats = h7_stats();
  if (!expect(stats.active_bytes >= 4096 + 16384 + 256, "active bytes after alloc") ||
      !expect(stats.span_count >= 1, "span count") ||
      !expect(stats.direct_count == 0, "direct count")) {
    return 1;
  }

  h7_free(b);
  h7_free(d);

  e2 = h7_malloc(32768);
  f2 = h7_malloc(65536);
  if (!expect(e2 == e, "reuse retained 32K direct") ||
      !expect(f2 == f, "reuse retained 64K direct") ||
      !expect(h7_route(e2) == H7_ROUTE_VALID, "route valid reused 32K") ||
      !expect(h7_route(f2) == H7_ROUTE_VALID, "route valid reused 64K")) {
    return 1;
  }

  h7_free(e2);
  h7_free(f2);
  h7_free(z);
  h7_free(0);

  stats = h7_stats();
  if (!expect(stats.active_bytes == 0, "active bytes after free") ||
      !expect(stats.direct_count == 0, "direct count after free")) {
    return 1;
  }

  a = h7_malloc(64);
  if (!expect(a != 0, "reuse malloc 64")) {
    return 1;
  }
  h7_free(a);

  if (!expect(h7_calloc((size_t)-1, 2) == 0, "calloc overflow")) {
    return 1;
  }

  printf("hz7-smoke ok\n");
  return 0;
}