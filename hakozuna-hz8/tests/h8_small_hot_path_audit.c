#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(HZ_SMALL_USE_HZ8)
#include "h8.h"
#elif defined(HZ_SMALL_USE_HZ11)
#include "hz11.h"
#endif

typedef void* (*HzSmallMallocFn)(size_t size);
typedef void (*HzSmallFreeFn)(void* ptr);

#if defined(HZ_SMALL_USE_TCMALLOC_DYNAMIC)
static HMODULE g_tcmalloc_module;
static HzSmallMallocFn g_tcmalloc_malloc;
static HzSmallFreeFn g_tcmalloc_free;
#endif

static int hz_small_init(void) {
#if defined(HZ_SMALL_USE_TCMALLOC_DYNAMIC)
  g_tcmalloc_module = LoadLibraryA("tcmalloc_minimal.dll");
  if (!g_tcmalloc_module) return 0;
  g_tcmalloc_malloc =
      (HzSmallMallocFn)(uintptr_t)GetProcAddress(g_tcmalloc_module, "tc_malloc");
  g_tcmalloc_free =
      (HzSmallFreeFn)(uintptr_t)GetProcAddress(g_tcmalloc_module, "tc_free");
  return g_tcmalloc_malloc != NULL && g_tcmalloc_free != NULL;
#else
  return 1;
#endif
}

static void* hz_small_malloc(size_t size) {
#if defined(HZ_SMALL_USE_HZ8)
  return h8_malloc(size);
#elif defined(HZ_SMALL_USE_HZ11)
  return hz11_malloc(size);
#elif defined(HZ_SMALL_USE_TCMALLOC_DYNAMIC)
  return g_tcmalloc_malloc(size);
#else
  return malloc(size);
#endif
}

static void hz_small_free(void* ptr) {
#if defined(HZ_SMALL_USE_HZ8)
  h8_free(ptr);
#elif defined(HZ_SMALL_USE_HZ11)
  hz11_free(ptr);
#elif defined(HZ_SMALL_USE_TCMALLOC_DYNAMIC)
  g_tcmalloc_free(ptr);
#else
  free(ptr);
#endif
}

static uint64_t hz_small_ticks(void) {
  LARGE_INTEGER value;
  QueryPerformanceCounter(&value);
  return (uint64_t)value.QuadPart;
}

int main(int argc, char** argv) {
  size_t size = 128u;
  size_t iterations = 20000000u;
  size_t warmup = 4096u;
  size_t failures = 0u;
  volatile uintptr_t sink = 0u;
  LARGE_INTEGER frequency;

  if (argc > 1) size = (size_t)strtoull(argv[1], NULL, 10);
  if (argc > 2) iterations = (size_t)strtoull(argv[2], NULL, 10);
  if (argc > 3) warmup = (size_t)strtoull(argv[3], NULL, 10);
  if ((size != 64u && size != 128u && size != 256u) || iterations == 0u) {
    return 2;
  }
  if (!hz_small_init() || !QueryPerformanceFrequency(&frequency)) return 3;
  if (SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1u) == 0u) return 4;

  for (size_t i = 0; i < warmup; ++i) {
    void* ptr = hz_small_malloc(size);
    if (!ptr) return 5;
    ((volatile unsigned char*)ptr)[0] = (unsigned char)i;
    ((volatile unsigned char*)ptr)[size - 1u] = (unsigned char)(i >> 8u);
    sink ^= (uintptr_t)ptr;
    hz_small_free(ptr);
  }

  uint64_t begin = hz_small_ticks();
  for (size_t i = 0; i < iterations; ++i) {
    void* ptr = hz_small_malloc(size);
    if (!ptr) {
      ++failures;
      continue;
    }
    ((volatile unsigned char*)ptr)[0] = (unsigned char)i;
    ((volatile unsigned char*)ptr)[size - 1u] = (unsigned char)(i >> 8u);
    sink ^= (uintptr_t)ptr;
    hz_small_free(ptr);
  }
  uint64_t end = hz_small_ticks();

  {
    double tick_ns = 1.0e9 / (double)frequency.QuadPart;
    double ns_pair = (double)(end - begin) * tick_ns / (double)iterations;
    printf("size=%zu iterations=%zu ns_pair=%.3f failures=%zu sink=%llu\n",
           size, iterations, ns_pair, failures,
           (unsigned long long)sink);
  }
  return failures == 0u ? 0 : 6;
}
