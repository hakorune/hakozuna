#include "../hz7.h"

#include <stdio.h>
#include <string.h>

static bool expect(bool cond, const char* label) {
  if (!cond) {
    fprintf(stderr, "hz7 cpp smoke failed: %s\n", label);
    return false;
  }
  return true;
}

int main() {
  void* small = h7_malloc(128);
  void* direct = h7_malloc(32768);
  H7Stats stats;

  if (!expect(small != nullptr, "malloc small") ||
      !expect(direct != nullptr, "malloc direct")) {
    return 1;
  }
  if (!expect(h7_route(small) == H7_ROUTE_VALID, "route valid small") ||
      !expect(h7_route(direct) == H7_ROUTE_VALID, "route valid direct")) {
    return 1;
  }

  memset(small, 0x11, 128);
  memset(direct, 0x22, 32768);

  stats = h7_stats();
  if (!expect(stats.active_bytes >= 128 + 32768, "active bytes") ||
      !expect(stats.direct_count == 1, "direct count") ||
      !expect(stats.route_register_fail == 0, "route register fail")) {
    return 1;
  }

  h7_free(small);
  h7_free(direct);

  stats = h7_stats();
  if (!expect(stats.active_bytes == 0, "active bytes after free") ||
      !expect(stats.direct_count == 0, "direct count after free") ||
      !expect(h7_route(direct) == H7_ROUTE_INVALID,
              "retained direct invalid after free")) {
    return 1;
  }

  printf("hz7-cpp-smoke ok\n");
  return 0;
}
