#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct WorkerArgs {
  uint64_t iters;
  size_t nmemb;
  size_t elem_size;
  int verify_zero;
  uint64_t ops;
  uint64_t checksum;
} WorkerArgs;

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static size_t read_current_rss_kb(void) {
  FILE* file = fopen("/proc/self/status", "r");
  if (!file) {
    return 0;
  }
  char line[256];
  size_t rss_kb = 0;
  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      (void)sscanf(line + 6, "%zu", &rss_kb);
      break;
    }
  }
  fclose(file);
  return rss_kb;
}

static void* worker_main(void* arg) {
  WorkerArgs* args = (WorkerArgs*)arg;
  const size_t bytes = args->nmemb * args->elem_size;
  for (uint64_t i = 0; i < args->iters; ++i) {
    unsigned char* ptr = (unsigned char*)calloc(args->nmemb, args->elem_size);
    if (!ptr) {
      fprintf(stderr, "calloc failed iter=%llu bytes=%zu\n",
              (unsigned long long)i, bytes);
      abort();
    }
    if (args->verify_zero && bytes != 0) {
      args->checksum += ptr[0];
      args->checksum += ptr[bytes / 2u];
      args->checksum += ptr[bytes - 1u];
    }
    if (bytes != 0) {
      ptr[0] = (unsigned char)i;
      ptr[bytes - 1u] = (unsigned char)(i >> 8);
    }
    free(ptr);
    args->ops += 2u;
  }
  return NULL;
}

int main(int argc, char** argv) {
  int threads = argc > 1 ? atoi(argv[1]) : 1;
  uint64_t iters = argc > 2 ? strtoull(argv[2], NULL, 10) : 100000;
  size_t nmemb = argc > 3 ? strtoull(argv[3], NULL, 10) : 1u;
  size_t elem_size = argc > 4 ? strtoull(argv[4], NULL, 10) : 4096u;
  int verify_zero = argc > 5 ? atoi(argv[5]) != 0 : 1;
  if (threads <= 0 || iters == 0 || nmemb == 0 || elem_size == 0 ||
      nmemb > ((size_t)-1) / elem_size) {
    fprintf(stderr,
            "usage: bench_calloc_ws [threads] [iters] [nmemb] [elem_size] "
            "[verify_zero]\n");
    return 2;
  }

  pthread_t* tids = (pthread_t*)calloc((size_t)threads, sizeof(*tids));
  WorkerArgs* args = (WorkerArgs*)calloc((size_t)threads, sizeof(*args));
  if (!tids || !args) {
    free(tids);
    free(args);
    return 3;
  }

  double start = now_sec();
  for (int i = 0; i < threads; ++i) {
    args[i].iters = iters;
    args[i].nmemb = nmemb;
    args[i].elem_size = elem_size;
    args[i].verify_zero = verify_zero;
    if (pthread_create(&tids[i], NULL, worker_main, &args[i]) != 0) {
      free(tids);
      free(args);
      return 4;
    }
  }
  uint64_t ops = 0;
  uint64_t checksum = 0;
  for (int i = 0; i < threads; ++i) {
    pthread_join(tids[i], NULL);
    ops += args[i].ops;
    checksum += args[i].checksum;
  }
  double elapsed = now_sec() - start;
  size_t current_kb = read_current_rss_kb();
  printf("threads=%d iters=%llu nmemb=%zu elem_size=%zu bytes=%zu "
         "verify_zero=%d time=%.6f ops/s=%.3f current_kb=%zu checksum=%llu\n",
         threads, (unsigned long long)iters, nmemb, elem_size,
         nmemb * elem_size, verify_zero, elapsed, (double)ops / elapsed,
         current_kb, (unsigned long long)checksum);

  free(args);
  free(tids);
  return 0;
}
