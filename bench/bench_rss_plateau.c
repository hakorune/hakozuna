#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static size_t current_rss_kb(void) {
  FILE* fp = fopen("/proc/self/status", "r");
  if (!fp) {
    return 0;
  }
  char line[256];
  size_t rss = 0;
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      (void)sscanf(line + 6, "%zu", &rss);
      break;
    }
  }
  fclose(fp);
  return rss;
}

static void touch_resident_block(void* ptr, size_t size, uint64_t salt) {
  const size_t page = 4096u;
  volatile unsigned char* bytes = (volatile unsigned char*)ptr;
  for (size_t offset = 0; offset < size; offset += page) {
    bytes[offset] = (unsigned char)(salt + offset);
  }
  bytes[size - 1u] = (unsigned char)(salt >> 8);
}

int main(int argc, char** argv) {
  size_t blocks = argc > 1 ? strtoull(argv[1], NULL, 10) : 1024u;
  int rounds = argc > 2 ? atoi(argv[2]) : 5;
  size_t size = argc > 3 ? strtoull(argv[3], NULL, 10) : 65536u;
  size_t align = argc > 4 ? strtoull(argv[4], NULL, 10) : 8192u;
  unsigned idle_ms = argc > 5 ? (unsigned)strtoul(argv[5], NULL, 10) : 0u;
  if (blocks == 0 || rounds <= 0 || size == 0 || align == 0 ||
      (align & (align - 1u)) != 0) {
    fprintf(stderr,
            "usage: bench_rss_plateau [blocks] [rounds] [size] [align] "
            "[idle_ms]\n");
    return 2;
  }

  void** ptrs = (void**)calloc(blocks, sizeof(*ptrs));
  if (!ptrs) {
    return 3;
  }

  size_t start_rss = current_rss_kb();
  size_t peak_rss = start_rss;
  size_t after_alloc_rss = start_rss;
  size_t after_free_rss = start_rss;
  uint64_t ops = 0;
  double start = now_sec();
  for (int round = 0; round < rounds; ++round) {
    for (size_t i = 0; i < blocks; ++i) {
      void* ptr = NULL;
      int rc = posix_memalign(&ptr, align, size);
      if (rc != 0 || !ptr) {
        fprintf(stderr, "posix_memalign failed rc=%d round=%d block=%zu\n",
                rc, round, i);
        free(ptrs);
        return 5;
      }
      touch_resident_block(ptr, size, (uint64_t)round * blocks + i);
      ptrs[i] = ptr;
      ops++;
    }
    after_alloc_rss = current_rss_kb();
    if (after_alloc_rss > peak_rss) {
      peak_rss = after_alloc_rss;
    }
    for (size_t i = 0; i < blocks; ++i) {
      free(ptrs[i]);
      ptrs[i] = NULL;
      ops++;
    }
    if (idle_ms) {
      struct timespec idle;
      idle.tv_sec = (time_t)(idle_ms / 1000u);
      idle.tv_nsec = (long)(idle_ms % 1000u) * 1000000L;
      nanosleep(&idle, NULL);
    }
    after_free_rss = current_rss_kb();
    if (after_free_rss > peak_rss) {
      peak_rss = after_free_rss;
    }
  }
  double elapsed = now_sec() - start;
  size_t final_rss = current_rss_kb();

  printf("allocator=preload mode=rss-plateau blocks=%zu rounds=%d "
         "size=%zu align=%zu idle_ms=%u time=%.6f ops/s=%.3f "
         "rss_start_kb=%zu rss_after_alloc_kb=%zu rss_after_free_kb=%zu "
         "rss_final_kb=%zu rss_peak_kb=%zu\n",
         blocks, rounds, size, align, idle_ms, elapsed, (double)ops / elapsed,
         start_rss, after_alloc_rss, after_free_rss, final_rss, peak_rss);
  free(ptrs);
  return 0;
}
