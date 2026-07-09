#ifndef HZ11_PORT_H
#define HZ11_PORT_H

#if defined(_MSC_VER) && !defined(__clang__)
#define HZ11_THREAD_LOCAL __declspec(thread)
#else
#define HZ11_THREAD_LOCAL _Thread_local
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef CRITICAL_SECTION HZ11_MUTEX;

static inline void hz11_mutex_init(HZ11_MUTEX* m) {
  InitializeCriticalSection(m);
}

static inline void hz11_mutex_lock(HZ11_MUTEX* m) {
  EnterCriticalSection(m);
}

static inline void hz11_mutex_unlock(HZ11_MUTEX* m) {
  LeaveCriticalSection(m);
}

/* Windows bring-up shim for the Linux-origin HZ11 middle-end code.
 * Keep this intentionally tiny: enough for mutex/once users in transfer-cache
 * and current-span diagnostics, not a general pthread compatibility layer. */
typedef CRITICAL_SECTION pthread_mutex_t;
typedef INIT_ONCE pthread_once_t;
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT

static BOOL CALLBACK hz11_pthread_once_callback(PINIT_ONCE init_once,
                                                PVOID parameter,
                                                PVOID* context) {
  (void)init_once;
  (void)context;
  ((void (*)(void))parameter)();
  return TRUE;
}

static inline int pthread_once(pthread_once_t* once_control,
                               void (*init_routine)(void)) {
  return InitOnceExecuteOnce(once_control,
                             hz11_pthread_once_callback,
                             (PVOID)init_routine,
                             NULL) ? 0 : -1;
}

static inline int pthread_mutex_init(pthread_mutex_t* m, void* attr) {
  (void)attr;
  InitializeCriticalSection(m);
  return 0;
}

static inline int pthread_mutex_lock(pthread_mutex_t* m) {
  EnterCriticalSection(m);
  return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t* m) {
  LeaveCriticalSection(m);
  return 0;
}
#else
#include <pthread.h>

typedef pthread_mutex_t HZ11_MUTEX;

static inline void hz11_mutex_init(HZ11_MUTEX* m) {
  (void)pthread_mutex_init(m, NULL);
}

static inline void hz11_mutex_lock(HZ11_MUTEX* m) {
  (void)pthread_mutex_lock(m);
}

static inline void hz11_mutex_unlock(HZ11_MUTEX* m) {
  (void)pthread_mutex_unlock(m);
}
#endif

#endif /* HZ11_PORT_H */
