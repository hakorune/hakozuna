#include "../hz7.h"

#include <stdio.h>

static int expect(int cond, const char* label) {
  if (!cond) {
    fprintf(stderr, "hz7 stats smoke failed: %s\n", label);
    return 0;
  }
  return 1;
}

int main(void) {
  void* small = h7_malloc(64);
  void* direct32 = h7_malloc(32768);
  void* direct64 = h7_malloc(65536);
  void* direct32_reuse;
  void* direct64_reuse;
  H7Stats stats;

  if (!expect(small != 0, "malloc small") ||
      !expect(direct32 != 0, "malloc 32K direct") ||
      !expect(direct64 != 0, "malloc 64K direct")) {
    return 1;
  }
  if (!expect(h7_route(small) == H7_ROUTE_VALID, "small route valid") ||
      !expect(h7_route(direct32) == H7_ROUTE_VALID, "32K route valid") ||
      !expect(h7_route(direct64) == H7_ROUTE_VALID, "64K route valid")) {
    return 1;
  }

  stats = h7_stats();
  if (!expect(stats.active_bytes >= 64 + 32768 + 65536,
              "active bytes include active allocations") ||
      !expect(stats.direct_count == 2, "direct count after alloc") ||
      !expect(stats.route_count >= 3, "route count after alloc") ||
      !expect(stats.route_register_fail == 0, "no route register fail")) {
    return 1;
  }

  h7_free(direct32);
  h7_free(direct64);
  if (!expect(h7_route(direct32) == H7_ROUTE_INVALID,
              "retained 32K route invalid") ||
      !expect(h7_route(direct64) == H7_ROUTE_INVALID,
              "retained 64K route invalid")) {
    return 1;
  }

  stats = h7_stats();
  if (!expect(stats.active_bytes >= 64, "small remains active") ||
      !expect(stats.direct_count == 0, "retained direct not active") ||
      !expect(stats.route_count >= 3, "retained routes remain registered") ||
      !expect(stats.route_register_fail == 0,
              "no route register fail after retain")) {
    return 1;
  }

  direct32_reuse = h7_malloc(32768);
  direct64_reuse = h7_malloc(65536);
  if (!expect(direct32_reuse == direct32, "reuse retained 32K") ||
      !expect(direct64_reuse == direct64, "reuse retained 64K") ||
      !expect(h7_route(direct32_reuse) == H7_ROUTE_VALID,
              "reused 32K route valid") ||
      !expect(h7_route(direct64_reuse) == H7_ROUTE_VALID,
              "reused 64K route valid")) {
    return 1;
  }

  stats = h7_stats();
  if (!expect(stats.direct_count == 2, "direct count after reuse") ||
      !expect(stats.active_bytes >= 64 + 32768 + 65536,
              "active bytes after reuse") ||
      !expect(stats.route_register_fail == 0,
              "no route register fail after reuse")) {
    return 1;
  }

  h7_free(small);
  h7_free(direct32_reuse);
  h7_free(direct64_reuse);

  stats = h7_stats();
  if (!expect(stats.active_bytes == 0, "active bytes after final free") ||
      !expect(stats.direct_count == 0, "direct count after final free") ||
      !expect(stats.route_count >= 1,
              "retained allocator routes may remain registered") ||
      !expect(stats.route_register_fail == 0, "no route register fail final")) {
    return 1;
  }

  printf("hz7-stats-smoke ok\n");
  return 0;
}
