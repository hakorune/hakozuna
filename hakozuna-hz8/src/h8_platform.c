#include "h8_platform.h"

#if defined(_WIN32)

#include <profileapi.h>

static LARGE_INTEGER h8_platform_qpc_freq;

static BOOL CALLBACK h8_platform_once_callback(PINIT_ONCE once, void* param,
                                               void** context) {
  (void)once;
  (void)context;
  void (*fn)(void) = (void (*)(void))param;
  fn();
  return TRUE;
}

static BOOL CALLBACK h8_platform_qpc_once_callback(PINIT_ONCE once, void* param,
                                                   void** context) {
  (void)once;
  (void)param;
  (void)context;
  return QueryPerformanceFrequency(&h8_platform_qpc_freq);
}

int h8_platform_once(h8_platform_once_t* once, void (*fn)(void)) {
  return InitOnceExecuteOnce(once, h8_platform_once_callback, fn, NULL) ? 0 : -1;
}

int h8_platform_thread_key_create(h8_platform_thread_key_t* key,
                                  void (*destructor)(void*)) {
  DWORD slot = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
  if (slot == FLS_OUT_OF_INDEXES) {
    return -1;
  }
  *key = slot;
  return 0;
}

void* h8_platform_thread_getspecific(h8_platform_thread_key_t key) {
  return FlsGetValue(key);
}

int h8_platform_thread_setspecific(h8_platform_thread_key_t key, void* value) {
  return FlsSetValue(key, value) ? 0 : -1;
}

int h8_platform_mutex_init(h8_platform_mutex_t* mutex) {
  InitializeSRWLock(mutex);
  return 0;
}

int h8_platform_mutex_destroy(h8_platform_mutex_t* mutex) {
  (void)mutex;
  return 0;
}

int h8_platform_mutex_lock(h8_platform_mutex_t* mutex) {
  AcquireSRWLockExclusive(mutex);
  return 0;
}

int h8_platform_mutex_unlock(h8_platform_mutex_t* mutex) {
  ReleaseSRWLockExclusive(mutex);
  return 0;
}

int h8_platform_mutex_trylock(h8_platform_mutex_t* mutex) {
  return TryAcquireSRWLockExclusive(mutex) ? 0 : -1;
}

void h8_platform_yield(void) {
  if (!SwitchToThread()) {
    Sleep(0);
  }
}

uint64_t h8_platform_now_ns(void) {
  static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
  LARGE_INTEGER counter;
  InitOnceExecuteOnce(&once, h8_platform_qpc_once_callback, NULL, NULL);
  QueryPerformanceCounter(&counter);
  return (uint64_t)((counter.QuadPart * 1000000000ull) /
                    h8_platform_qpc_freq.QuadPart);
}

void* h8_platform_reserve(size_t bytes) {
  return VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_READWRITE);
}

int h8_platform_commit(void* ptr, size_t bytes) {
  return VirtualAlloc(ptr, bytes, MEM_COMMIT, PAGE_READWRITE) ? 0 : -1;
}

void* h8_platform_reserve_rw(size_t bytes) {
  return VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void h8_platform_release(void* ptr, size_t bytes) {
  (void)bytes;
  if (ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
  }
}

int h8_platform_purge(void* ptr, size_t bytes) {
  return VirtualAlloc(ptr, bytes, MEM_RESET, PAGE_READWRITE) ? 0 : -1;
}

#else

#include <sched.h>
#include <sys/mman.h>
#include <time.h>

int h8_platform_once(h8_platform_once_t* once, void (*fn)(void)) {
  return pthread_once(once, fn);
}

int h8_platform_thread_key_create(h8_platform_thread_key_t* key,
                                  void (*destructor)(void*)) {
  return pthread_key_create(key, destructor);
}

void* h8_platform_thread_getspecific(h8_platform_thread_key_t key) {
  return pthread_getspecific(key);
}

int h8_platform_thread_setspecific(h8_platform_thread_key_t key, void* value) {
  return pthread_setspecific(key, value);
}

int h8_platform_mutex_init(h8_platform_mutex_t* mutex) {
  return pthread_mutex_init(mutex, NULL);
}

int h8_platform_mutex_destroy(h8_platform_mutex_t* mutex) {
  return pthread_mutex_destroy(mutex);
}

int h8_platform_mutex_lock(h8_platform_mutex_t* mutex) {
  return pthread_mutex_lock(mutex);
}

int h8_platform_mutex_unlock(h8_platform_mutex_t* mutex) {
  return pthread_mutex_unlock(mutex);
}

int h8_platform_mutex_trylock(h8_platform_mutex_t* mutex) {
  return pthread_mutex_trylock(mutex);
}

void h8_platform_yield(void) {
  sched_yield();
}

uint64_t h8_platform_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void* h8_platform_reserve(size_t bytes) {
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
  mmap_flags |= MAP_NORESERVE;
#endif
  void* ptr = mmap(NULL, bytes, PROT_NONE, mmap_flags, -1, 0);
  return ptr == MAP_FAILED ? NULL : ptr;
}

int h8_platform_commit(void* ptr, size_t bytes) {
  return mprotect(ptr, bytes, PROT_READ | PROT_WRITE);
}

void* h8_platform_reserve_rw(size_t bytes) {
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
  mmap_flags |= MAP_NORESERVE;
#endif
  void* ptr = mmap(NULL, bytes, PROT_READ | PROT_WRITE, mmap_flags, -1, 0);
  return ptr == MAP_FAILED ? NULL : ptr;
}

void h8_platform_release(void* ptr, size_t bytes) {
  if (ptr && bytes) {
    munmap(ptr, bytes);
  }
}

int h8_platform_purge(void* ptr, size_t bytes) {
  return madvise(ptr, bytes, MADV_DONTNEED);
}

#endif
