#define _GNU_SOURCE

#include "hz3_config.h"

#if HZ3_DEBUG_SEGV_BACKTRACE

#include <execinfo.h>
#include <dlfcn.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>
#include <unistd.h>

static uintptr_t g_hz3_so_base = 0;

static void hz3_debug_segv_write_hex_kv(const char* key, uintptr_t val) {
    char buf[192];
    int n = 0;
    buf[n++] = '[';
    buf[n++] = 'H';
    buf[n++] = 'Z';
    buf[n++] = '3';
    buf[n++] = '_';
    buf[n++] = 'S';
    buf[n++] = 'E';
    buf[n++] = 'G';
    buf[n++] = 'V';
    buf[n++] = '_';
    buf[n++] = 'B';
    buf[n++] = 'T';
    buf[n++] = ']';
    buf[n++] = ' ';

    for (const char* p = key; *p && n < (int)sizeof(buf) - 1; p++) {
        buf[n++] = *p;
    }
    buf[n++] = '=';
    buf[n++] = '0';
    buf[n++] = 'x';

    static const char hex[] = "0123456789abcdef";
    for (int i = (int)(sizeof(uintptr_t) * 2) - 1; i >= 0; i--) {
        buf[n++] = hex[(val >> (i * 4)) & 0xFu];
    }
    buf[n++] = '\n';
    ssize_t wr = write(STDERR_FILENO, buf, (size_t)n);
    (void)wr;
}

static void hz3_debug_segv_handler(int sig, siginfo_t* info, void* uctx) {
    const char hdr[] = "[HZ3_SEGV_BT] signal\n";
    {
        ssize_t wr = write(STDERR_FILENO, hdr, sizeof(hdr) - 1);
        (void)wr;
    }

    if (info) {
        hz3_debug_segv_write_hex_kv("addr", (uintptr_t)info->si_addr);
    }

    // Best-effort: print instruction pointer from ucontext (outside of backtrace()).
    if (uctx) {
        uintptr_t ip = 0;
#if defined(__x86_64__)
        ucontext_t* uc = (ucontext_t*)uctx;
        ip = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__aarch64__)
        ucontext_t* uc = (ucontext_t*)uctx;
        ip = (uintptr_t)uc->uc_mcontext.pc;
#endif
        if (ip) {
            hz3_debug_segv_write_hex_kv("ip", ip);
            if (g_hz3_so_base && ip >= g_hz3_so_base) {
                hz3_debug_segv_write_hex_kv("ip_off", (uintptr_t)(ip - g_hz3_so_base));
            }
        }
    }

    void* frames[64];
    int count = backtrace(frames, (int)(sizeof(frames) / sizeof(frames[0])));
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    _exit(128 + sig);
}

__attribute__((constructor))
static void hz3_debug_segv_install(void) {
    // Cache our .so base address outside the signal handler so we can compute ip_off safely.
    {
        Dl_info di;
        if (dladdr((void*)&hz3_debug_segv_install, &di) && di.dli_fbase) {
            g_hz3_so_base = (uintptr_t)di.dli_fbase;
        }
    }

    struct sigaction sa;
    sa.sa_sigaction = hz3_debug_segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &sa, NULL);
}

#endif  // HZ3_DEBUG_SEGV_BACKTRACE
