#pragma once

struct group {
    char* gr_name;
    char* gr_passwd;
    int gr_gid;
    char** gr_mem;
};

static inline struct group* getgrnam(const char* name) {
    (void)name;
    return 0;
}
