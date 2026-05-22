#include "hz5.h"
#include "hz5_wrapper.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BENCHLAB_HZ5_LINUX_LOCAL2P
#define BENCHLAB_HZ5_LINUX_LOCAL2P 0
#endif

typedef struct RemoteFreeArgs {
  void* ptr;
  Hz5FreeResult result;
} RemoteFreeArgs;

static Hz5WrapperHdr* hdr_from_ptr(void* ptr) {
  return (Hz5WrapperHdr*)((uintptr_t)ptr - sizeof(Hz5WrapperHdr));
}

static int expect(int condition, const char* name) {
  if (!condition) {
    fprintf(stderr, "safety failed: %s\n", name);
    return 1;
  }
  return 0;
}

static void* remote_free_main(void* arg) {
  RemoteFreeArgs* args = (RemoteFreeArgs*)arg;
  args->result = hz5_free(args->ptr);
  return NULL;
}

int main(void) {
  int failures = 0;

  void* ptr = hz5_aligned_alloc(65536u, 8192u);
  failures += expect(ptr != NULL, "valid alloc");
  if (ptr) {
    failures += expect(hz5_free(ptr) != HZ5_FREE_INVALID, "valid free");
  }

#if BENCHLAB_HZ5_LINUX_LOCAL2P
  ptr = hz5_aligned_alloc(65536u, 8192u);
  failures += expect(ptr != NULL, "double-free alloc");
  if (ptr) {
    failures += expect(hz5_free(ptr) != HZ5_FREE_INVALID,
                       "double-free first free");
    failures += expect(hz5_free(ptr) == HZ5_FREE_INVALID,
                       "double-free rejected");
  }

  ptr = hz5_aligned_alloc(65536u, 8192u);
  failures += expect(ptr != NULL, "mutated cookie alloc");
  if (ptr) {
    Hz5WrapperHdr* hdr = hdr_from_ptr(ptr);
    hdr->local2p_cookie ^= UINT64_C(0x55);
    failures += expect(hz5_free(ptr) == HZ5_FREE_INVALID,
                       "mutated cookie rejected");
  }

  ptr = hz5_aligned_alloc(65536u, 8192u);
  failures += expect(ptr != NULL, "wrong source alloc");
  if (ptr) {
    Hz5WrapperHdr* hdr = hdr_from_ptr(ptr);
    hdr->source = HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE;
    failures += expect(hz5_free(ptr) == HZ5_FREE_INVALID,
                       "wrong source rejected");
  }

  int foreign = 0;
  failures += expect(hz5_free(&foreign) == HZ5_FREE_INVALID,
                     "foreign pointer rejected");

  ptr = hz5_aligned_alloc(65536u, 8192u);
  failures += expect(ptr != NULL, "remote free alloc");
  if (ptr) {
    pthread_t tid;
    RemoteFreeArgs args = {ptr, HZ5_FREE_INVALID};
    failures += expect(pthread_create(&tid, NULL, remote_free_main, &args) == 0,
                       "remote thread create");
    pthread_join(tid, NULL);
    failures += expect(args.result != HZ5_FREE_INVALID,
                       "remote free accepted via OS release");
  }
#endif

  failures += expect(hz5_aligned_alloc(65537u, 16u) == NULL,
                     "unsupported 65537:16 rejected");
  failures += expect(hz5_aligned_alloc(65536u, 4096u) == NULL,
                     "unsupported 65536:4096 rejected");

  if (failures != 0) {
    return 1;
  }
  puts("hz5-standalone-safety ok");
  return 0;
}
