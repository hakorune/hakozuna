#pragma once

#ifndef RLIMIT_NOFILE
#define RLIMIT_NOFILE 7
#endif

#ifndef RLIMIT_CORE
#define RLIMIT_CORE 4
#endif

#ifndef RLIM_INFINITY
#define RLIM_INFINITY ((unsigned long)-1)
#endif

struct rlimit {
    unsigned long rlim_cur;
    unsigned long rlim_max;
};

static inline int getrlimit(int resource, struct rlimit* rlim) {
    (void)resource;
    if (!rlim) {
        return -1;
    }
    rlim->rlim_cur = 0;
    rlim->rlim_max = 0;
    return 0;
}

static inline int setrlimit(int resource, const struct rlimit* rlim) {
    (void)resource;
    (void)rlim;
    return 0;
}
