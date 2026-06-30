#include <dlfcn.h>
#include <errno.h>
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

__attribute__((noinline)) static void* realloc_offset(char* p, size_t offset,
                                                       size_t size) {
  volatile uintptr_t base = (uintptr_t)p;
  void* q = (void*)(base + offset);
  return realloc(q, size);
}

static H8RouteFn load_route(void) {
  void* sym = dlsym(RTLD_DEFAULT, "h8_route");
  if (!sym) {
    fprintf(stderr, "h8_route not visible through preload\n");
    return NULL;
  }
  return (H8RouteFn)sym;
}

__attribute__((noinline)) static int route_addr(H8RouteFn route,
                                                uintptr_t addr) {
  volatile uintptr_t v = addr;
  return route((void*)v);
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
  uintptr_t p_addr = (uintptr_t)p;
  free(p);
  if (route_addr(route, p_addr) == H8_ROUTE_VALID) {
    fprintf(stderr, "base stayed valid after free\n");
    return 5;
  }

  char* r = realloc(NULL, 48);
  if (!r) {
    fprintf(stderr, "realloc NULL failed\n");
    return 10;
  }
  for (size_t i = 0; i < 48; ++i) {
    r[i] = (char)(0x30 + (int)(i & 15u));
  }
  char* rgrow = realloc(r, 200);
  if (!rgrow) {
    fprintf(stderr, "small realloc grow failed\n");
    return 11;
  }
  for (size_t i = 0; i < 48; ++i) {
    if (rgrow[i] != (char)(0x30 + (int)(i & 15u))) {
      fprintf(stderr, "small realloc grow lost byte %zu\n", i);
      return 12;
    }
  }
  char* rshrink = realloc(rgrow, 24);
  if (!rshrink) {
    fprintf(stderr, "small realloc shrink failed\n");
    return 13;
  }
  for (size_t i = 0; i < 24; ++i) {
    if (rshrink[i] != (char)(0x30 + (int)(i & 15u))) {
      fprintf(stderr, "small realloc shrink lost byte %zu\n", i);
      return 14;
    }
  }
  if (realloc(rshrink, 0) != NULL) {
    fprintf(stderr, "realloc size zero did not return NULL\n");
    return 15;
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

  char* m = malloc(9000);
  if (!m) {
    fprintf(stderr, "medium malloc failed\n");
    return 16;
  }
  memset(m, 0x5A, 9000);
  char* mgrow = realloc(m, 20000);
  if (!mgrow) {
    fprintf(stderr, "medium realloc grow failed\n");
    return 17;
  }
  for (size_t i = 0; i < 9000; ++i) {
    if ((unsigned char)mgrow[i] != 0x5Au) {
      fprintf(stderr, "medium realloc lost byte %zu\n", i);
      return 18;
    }
  }
  free(mgrow);

  char* bad = malloc(96);
  if (!bad) {
    fprintf(stderr, "invalid realloc setup failed\n");
    return 19;
  }
  errno = 0;
  if (realloc_offset(bad, 1, 128) != NULL || errno != EINVAL) {
    fprintf(stderr, "interior realloc did not fail closed errno=%d\n", errno);
    return 20;
  }
  if (route(bad) != H8_ROUTE_VALID) {
    fprintf(stderr, "interior realloc consumed base allocation\n");
    return 21;
  }
  free(bad);

  void* large = malloc(256 * 1024);
  if (!large) {
    fprintf(stderr, "large malloc failed\n");
    return 8;
  }
  if (route(large) != H8_ROUTE_MISS) {
    fprintf(stderr, "large platform allocation was not MISS\n");
    return 9;
  }
  large = realloc(large, 512 * 1024);
  if (!large) {
    fprintf(stderr, "large platform realloc failed\n");
    return 22;
  }
  if (route(large) != H8_ROUTE_MISS) {
    fprintf(stderr, "large platform realloc was not MISS\n");
    return 23;
  }
  free(large);

  printf("preload_smoke ok\n");
  return 0;
}
