#include "hz5_hz3_fallback.h"

#include "hz5_config.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#if !defined(_WIN32)
#error HZ5 lazy HZ3 fallback currently requires Windows LoadLibrary
#endif

#include <windows.h>

typedef void* (*Hz5Hz3FallbackAllocFn)(size_t size, size_t align);
typedef void (*Hz5Hz3FallbackFreeFn)(void* ptr);

static INIT_ONCE g_hz5_hz3_fb_once = INIT_ONCE_STATIC_INIT;
static _Atomic uint32_t g_hz5_hz3_fb_state = HZ5_HZ3_FALLBACK_UNLOADED;
static HMODULE g_hz5_hz3_fb_module;
static Hz5Hz3FallbackAllocFn g_hz5_hz3_fb_alloc;
static Hz5Hz3FallbackFreeFn g_hz5_hz3_fb_free;

static _Atomic size_t g_hz5_hz3_fb_load_count;
static _Atomic size_t g_hz5_hz3_fb_load_failed;
static _Atomic size_t g_hz5_hz3_fb_alloc_calls;
static _Atomic size_t g_hz5_hz3_fb_free_calls;
static _Atomic size_t g_hz5_hz3_fb_free_miss_unloaded;
static _Atomic size_t g_hz5_hz3_fb_free_loading_wait;
static _Atomic size_t g_hz5_hz3_fb_crt_bypass_alloc;
static _Atomic size_t g_hz5_hz3_fb_crt_bypass_free;

static void hz5_hz3_fallback_print_once(void) {
  static atomic_flag printed = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set_explicit(&printed, memory_order_acq_rel)) {
    return;
  }

  fprintf(stderr,
          "[HZ5_P20_LAZY_FALLBACK] state=%u load_count=%zu load_failed=%zu "
          "alloc_calls=%zu free_calls=%zu free_miss_unloaded=%zu "
          "free_loading_wait=%zu crt_bypass_alloc=%zu "
          "crt_bypass_free=%zu\n",
          atomic_load_explicit(&g_hz5_hz3_fb_state, memory_order_acquire),
          atomic_load_explicit(&g_hz5_hz3_fb_load_count,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_load_failed,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_alloc_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_free_calls,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_free_miss_unloaded,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_free_loading_wait,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_crt_bypass_alloc,
                               memory_order_relaxed),
          atomic_load_explicit(&g_hz5_hz3_fb_crt_bypass_free,
                               memory_order_relaxed));
}

void hz5_hz3_fallback_register_once(void) {
  static atomic_flag registered = ATOMIC_FLAG_INIT;
  if (!atomic_flag_test_and_set_explicit(&registered, memory_order_acq_rel)) {
    atexit(hz5_hz3_fallback_print_once);
  }
}

uint32_t hz5_hz3_fallback_state(void) {
  return atomic_load_explicit(&g_hz5_hz3_fb_state, memory_order_acquire);
}

static int hz5_hz3_fallback_build_path(wchar_t* out, size_t out_count) {
  HMODULE self = NULL;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCWSTR)&hz5_hz3_fallback_build_path, &self)) {
    return 0;
  }

  DWORD len = GetModuleFileNameW(self, out, (DWORD)out_count);
  if (len == 0 || len >= out_count) {
    return 0;
  }

  wchar_t* slash = NULL;
  for (wchar_t* p = out; *p; ++p) {
    if (*p == L'\\' || *p == L'/') {
      slash = p;
    }
  }
  if (!slash) {
    return 0;
  }
  slash[1] = L'\0';

  static const wchar_t dll_name[] = L"benchlab_hz3_body_speed_default.dll";
  size_t base_len = wcslen(out);
  size_t dll_len = sizeof(dll_name) / sizeof(dll_name[0]) - 1u;
  if (base_len + dll_len + 1u > out_count) {
    return 0;
  }
  memcpy(out + base_len, dll_name, (dll_len + 1u) * sizeof(wchar_t));
  return 1;
}

static BOOL CALLBACK hz5_hz3_fallback_init_once(PINIT_ONCE once,
                                                PVOID param,
                                                PVOID* context) {
  (void)once;
  (void)param;
  (void)context;

  atomic_store_explicit(&g_hz5_hz3_fb_state, HZ5_HZ3_FALLBACK_LOADING,
                        memory_order_release);

  wchar_t path[MAX_PATH];
  HMODULE module = NULL;
  if (hz5_hz3_fallback_build_path(path, sizeof(path) / sizeof(path[0]))) {
    module = LoadLibraryW(path);
  }
  if (!module) {
    module = LoadLibraryW(L"benchlab_hz3_body_speed_default.dll");
  }

  if (!module) {
    atomic_fetch_add_explicit(&g_hz5_hz3_fb_load_failed, 1,
                              memory_order_relaxed);
    atomic_store_explicit(&g_hz5_hz3_fb_state, HZ5_HZ3_FALLBACK_FAILED,
                          memory_order_release);
    return TRUE;
  }

  Hz5Hz3FallbackAllocFn alloc_fn =
      (Hz5Hz3FallbackAllocFn)GetProcAddress(module, "benchlab_alloc");
  Hz5Hz3FallbackFreeFn free_fn =
      (Hz5Hz3FallbackFreeFn)GetProcAddress(module, "benchlab_free");
  if (!alloc_fn || !free_fn) {
    atomic_fetch_add_explicit(&g_hz5_hz3_fb_load_failed, 1,
                              memory_order_relaxed);
    atomic_store_explicit(&g_hz5_hz3_fb_state, HZ5_HZ3_FALLBACK_FAILED,
                          memory_order_release);
    return TRUE;
  }

  g_hz5_hz3_fb_module = module;
  g_hz5_hz3_fb_alloc = alloc_fn;
  g_hz5_hz3_fb_free = free_fn;
  atomic_fetch_add_explicit(&g_hz5_hz3_fb_load_count, 1,
                            memory_order_relaxed);
  atomic_store_explicit(&g_hz5_hz3_fb_state, HZ5_HZ3_FALLBACK_READY,
                        memory_order_release);
  return TRUE;
}

static int hz5_hz3_fallback_ensure_loaded(void) {
  hz5_hz3_fallback_register_once();
  uint32_t state =
      atomic_load_explicit(&g_hz5_hz3_fb_state, memory_order_acquire);
  if (state == HZ5_HZ3_FALLBACK_READY) {
    return 1;
  }
  if (state == HZ5_HZ3_FALLBACK_FAILED) {
    return 0;
  }
  InitOnceExecuteOnce(&g_hz5_hz3_fb_once, hz5_hz3_fallback_init_once, NULL,
                      NULL);
  return atomic_load_explicit(&g_hz5_hz3_fb_state, memory_order_acquire) ==
         HZ5_HZ3_FALLBACK_READY;
}

void* hz5_hz3_fallback_alloc(size_t size, size_t align) {
#if HZ5_DIAGNOSTIC_STATS
  atomic_fetch_add_explicit(&g_hz5_hz3_fb_alloc_calls, 1,
                            memory_order_relaxed);
#endif
  if (!hz5_hz3_fallback_ensure_loaded()) {
    return NULL;
  }
  return g_hz5_hz3_fb_alloc(size, align);
}

void hz5_hz3_fallback_free(void* ptr) {
  uint32_t state =
      atomic_load_explicit(&g_hz5_hz3_fb_state, memory_order_acquire);
  if (state == HZ5_HZ3_FALLBACK_READY) {
#if HZ5_DIAGNOSTIC_STATS
    atomic_fetch_add_explicit(&g_hz5_hz3_fb_free_calls, 1,
                              memory_order_relaxed);
#endif
    g_hz5_hz3_fb_free(ptr);
    return;
  }
  if (state == HZ5_HZ3_FALLBACK_LOADING) {
    atomic_fetch_add_explicit(&g_hz5_hz3_fb_free_loading_wait, 1,
                              memory_order_relaxed);
    if (hz5_hz3_fallback_ensure_loaded()) {
#if HZ5_DIAGNOSTIC_STATS
      atomic_fetch_add_explicit(&g_hz5_hz3_fb_free_calls, 1,
                                memory_order_relaxed);
#endif
      g_hz5_hz3_fb_free(ptr);
      return;
    }
  }

  atomic_fetch_add_explicit(&g_hz5_hz3_fb_free_miss_unloaded, 1,
                            memory_order_relaxed);
}

void hz5_hz3_fallback_note_crt_bypass_alloc(void) {
  atomic_fetch_add_explicit(&g_hz5_hz3_fb_crt_bypass_alloc, 1,
                            memory_order_relaxed);
}

void hz5_hz3_fallback_note_crt_bypass_free(void) {
  atomic_fetch_add_explicit(&g_hz5_hz3_fb_crt_bypass_free, 1,
                            memory_order_relaxed);
}
