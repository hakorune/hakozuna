#pragma once

#if !defined(_WIN32)
#error "signal shim is Windows-only"
#endif

#include_next <signal.h>

#include <stddef.h>

#ifndef SIGBUS
#define SIGBUS 7
#endif

#ifndef SA_NODEFER
#define SA_NODEFER 0x00000001
#endif

#ifndef SA_ONSTACK
#define SA_ONSTACK 0x00000002
#endif

#ifndef SA_RESETHAND
#define SA_RESETHAND 0x00000004
#endif

#ifndef SA_SIGINFO
#define SA_SIGINFO 0x00000008
#endif

typedef int sigset_t;

typedef struct siginfo_s {
    int si_code;
    void *si_addr;
} siginfo_t;

struct sigaction {
    unsigned int sa_flags;
    sigset_t sa_mask;
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    };
};

static inline int sigemptyset(sigset_t *set) {
    if (!set) {
        return -1;
    }
    *set = 0;
    return 0;
}

static inline int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
    (void)sig;
    if (oldact && act) {
        *oldact = *act;
    }
    return 0;
}
