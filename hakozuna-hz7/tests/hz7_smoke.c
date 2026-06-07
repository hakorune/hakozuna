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

int main(void) {
  void* a = h7_malloc(64);
  void* b = h7_malloc(4096);
  void* c = h7_malloc(4097);
  unsigned char* z = (unsigned char*)h7_calloc(32, 8);
  H7Stats stats;
  size_t i;

  if (!expect(a != 0, "malloc 64") || !expect(b != 0, "malloc 4096") ||
      !expect(c != 0, "malloc 4097") || !expect(z != 0, "calloc")) {
    return 1;
  }
  for (i = 0; i < 32u * 8u; ++i) {
    if (!expect(z[i] == 0, "calloc zero fill")) {
      return 1;
    }
  }
  memset(a, 0xA5, 64);
  memset(b, 0x5A, 4096);
  memset(c, 0xC3, 4097);

  stats = h7_stats();
  if (!expect(stats.active_bytes >= 64 + 4096 + 4097 + 256,
              "active bytes after alloc")) {
    return 1;
  }
  if (!expect(stats.span_count >= 1, "span count") ||
      !expect(stats.direct_count == 1, "direct count")) {
    return 1;
  }

  h7_free(a);
  h7_free(b);
  h7_free(c);
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

