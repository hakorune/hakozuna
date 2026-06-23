#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  H8_ROUTE_MISS = 0,
  H8_ROUTE_VALID = 1,
  H8_ROUTE_INVALID = 2
};

typedef int (*H8RouteFn)(void*);

__attribute__((noinline)) static void free_offset(char* p, size_t offset) {
  volatile uintptr_t base = (uintptr_t)p;
  void* q = (void*)(base + offset);
  free(q);
}

static H8RouteFn load_route(void) {
  void* sym = dlsym(RTLD_DEFAULT, "h8_route");
  if (!sym) {
    fprintf(stderr, "h8_route not visible through preload\n");
    return NULL;
  }
  return (H8RouteFn)sym;
}

int main(void) {
  H8RouteFn route = load_route();
  if (!route) {
    return 1;
  }

  char* p = malloc(64);
  if (!p) {
    fprintf(stderr, "small malloc failed\n");
    return 2;
  }
  memset(p, 0xA5, 64);
  if (route(p) != H8_ROUTE_VALID || route(p + 1) != H8_ROUTE_INVALID ||
      route(p + 63) != H8_ROUTE_INVALID) {
    fprintf(stderr, "small route classification failed\n");
    return 3;
  }

  free_offset(p, 1);
  if (route(p) != H8_ROUTE_VALID) {
    fprintf(stderr, "interior free consumed base allocation\n");
    return 4;
  }
  free(p);
  if (route(p) == H8_ROUTE_VALID) {
    fprintf(stderr, "base stayed valid after free\n");
    return 5;
  }

  unsigned char* z = calloc(32, 4);
  if (!z) {
    fprintf(stderr, "calloc failed\n");
    return 6;
  }
  for (size_t i = 0; i < 128; ++i) {
    if (z[i] != 0) {
      fprintf(stderr, "calloc returned nonzero byte\n");
      return 7;
    }
  }
  free(z);

  void* large = malloc(8192);
  if (!large) {
    fprintf(stderr, "large malloc failed\n");
    return 8;
  }
  if (route(large) != H8_ROUTE_MISS) {
    fprintf(stderr, "large platform allocation was not MISS\n");
    return 9;
  }
  free(large);

  printf("preload_smoke ok\n");
  return 0;
}
