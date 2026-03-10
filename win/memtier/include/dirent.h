#pragma once

#if !defined(_WIN32)
#error "dirent shim is Windows-only"
#endif

struct dirent {
    char d_name[260];
};

typedef struct hz_memtier_dir_s DIR;

static inline DIR *opendir(const char *path) {
    (void)path;
    return (DIR *)0;
}

static inline struct dirent *readdir(DIR *dirp) {
    (void)dirp;
    return (struct dirent *)0;
}

static inline int closedir(DIR *dirp) {
    (void)dirp;
    return 0;
}
