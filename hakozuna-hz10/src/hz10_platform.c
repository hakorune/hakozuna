#include "hz10_platform.h"

#if defined(_WIN32)

#include <profileapi.h>

static LARGE_INTEGER hz10_platform_qpc_freq;

static BOOL CALLBACK hz10_platform_once_callback(PINIT_ONCE once, void* param,
                                                 void** context) {
  (void)once;
  (void)context;
  void (*fn)(void) = (void (*)(void))param;
  fn();
  return TRUE;
}

static BOOL CALLBACK hz10_platform_qpc_once_callback(PINIT_ONCE once,
                                                     void* param,
                                                     void** context) {
  (void)once;
  (void)param;
  (void)context;
  return QueryPerformanceFrequency(&hz10_platform_qpc_freq);
}

int hz10_platform_once(hz10_platform_once_t* once, void (*fn)(void)) {
  return InitOnceExecuteOnce(once, hz10_platform_once_callback, fn, NULL)
             ? 0
             : -1;
}

int hz10_platform_thread_key_create(hz10_platform_thread_key_t* key,
                                    void (*destructor)(void*)) {
  DWORD slot = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
  if (slot == FLS_OUT_OF_INDEXES) {
    return -1;
  }
  *key = slot;
  return 0;
}

void* hz10_platform_thread_getspecific(hz10_platform_thread_key_t key) {
  return FlsGetValue(key);
}

int hz10_platform_thread_setspecific(hz10_platform_thread_key_t key,
                                    void* value) {
  return FlsSetValue(key, value) ? 0 : -1;
}

int hz10_platform_mutex_init(hz10_platform_mutex_t* mutex) {
  InitializeSRWLock(mutex);
  return 0;
}

int hz10_platform_mutex_destroy(hz10_platform_mutex_t* mutex) {
  (void)mutex;
  return 0;
}

int hz10_platform_mutex_lock(hz10_platform_mutex_t* mutex) {
  AcquireSRWLockExclusive(mutex);
  return 0;
}

int hz10_platform_mutex_unlock(hz10_platform_mutex_t* mutex) {
  ReleaseSRWLockExclusive(mutex);
  return 0;
}

int hz10_platform_mutex_trylock(hz10_platform_mutex_t* mutex) {
  return TryAcquireSRWLockExclusive(mutex) ? 0 : -1;
}

void hz10_platform_yield(void) {
  if (!SwitchToThread()) {
    Sleep(0);
  }
}

uint64_t hz10_platform_now_ns(void) {
  static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
  LARGE_INTEGER counter;
  InitOnceExecuteOnce(&once, hz10_platform_qpc_once_callback, NULL, NULL);
  QueryPerformanceCounter(&counter);
  return (uint64_t)((counter.QuadPart * 1000000000ull) /
                    hz10_platform_qpc_freq.QuadPart);
}

void* hz10_platform_reserve(size_t bytes) {
  return VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_READWRITE);
}

int hz10_platform_commit(void* ptr, size_t bytes) {
  return VirtualAlloc(ptr, bytes, MEM_COMMIT, PAGE_READWRITE) ? 0 : -1;
}

void* hz10_platform_reserve_rw(size_t bytes) {
  return VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void hz10_platform_release(void* ptr, size_t bytes) {
  (void)bytes;
  if (ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
  }
}

int hz10_platform_purge(void* ptr, size_t bytes) {
  return VirtualAlloc(ptr, bytes, MEM_RESET, PAGE_READWRITE) ? 0 : -1;
}

#else

#include <sched.h>
#include <sys/mman.h>
#include <time.h>

int hz10_platform_once(hz10_platform_once_t* once, void (*fn)(void)) {
  return pthread_once(once, fn);
}

int hz10_platform_thread_key_create(hz10_platform_thread_key_t* key,
                                    void (*destructor)(void*)) {
  return pthread_key_create(key, destructor);
}

void* hz10_platform_thread_getspecific(hz10_platform_thread_key_t key) {
  return pthread_getspecific(key);
}

int hz10_platform_thread_setspecific(hz10_platform_thread_key_t key,
                                    void* value) {
  return pthread_setspecific(key, value);
}

int hz10_platform_mutex_init(hz10_platform_mutex_t* mutex) {
  return pthread_mutex_init(mutex, NULL);
}

int hz10_platform_mutex_destroy(hz10_platform_mutex_t* mutex) {
  return pthread_mutex_destroy(mutex);
}

int hz10_platform_mutex_lock(hz10_platform_mutex_t* mutex) {
  return pthread_mutex_lock(mutex);
}

int hz10_platform_mutex_unlock(hz10_platform_mutex_t* mutex) {
  return pthread_mutex_unlock(mutex);
}

int hz10_platform_mutex_trylock(hz10_platform_mutex_t* mutex) {
  return pthread_mutex_trylock(mutex);
}

void hz10_platform_yield(void) {
  sched_yield();
}

uint64_t hz10_platform_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void* hz10_platform_reserve(size_t bytes) {
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
  mmap_flags |= MAP_NORESERVE;
#endif
  void* ptr = mmap(NULL, bytes, PROT_NONE, mmap_flags, -1, 0);
  return ptr == MAP_FAILED ? NULL : ptr;
}

int hz10_platform_commit(void* ptr, size_t bytes) {
  return mprotect(ptr, bytes, PROT_READ | PROT_WRITE);
}

void* hz10_platform_reserve_rw(size_t bytes) {
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
  mmap_flags |= MAP_NORESERVE;
#endif
  void* ptr = mmap(NULL, bytes, PROT_READ | PROT_WRITE, mmap_flags, -1, 0);
  return ptr == MAP_FAILED ? NULL : ptr;
}

void hz10_platform_release(void* ptr, size_t bytes) {
  if (ptr && bytes) {
    munmap(ptr, bytes);
  }
}

int hz10_platform_purge(void* ptr, size_t bytes) {
  return madvise(ptr, bytes, MADV_DONTNEED);
}

#endif
