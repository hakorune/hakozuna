#include <dlfcn.h>
#include <malloc/malloc.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef size_t (*probe_size_fn_t)(void*);

static probe_size_fn_t resolve_size_fn(const char** symbol_out) {
    const char* symbol = getenv("HKO_SIZE_SYMBOL");
    if (symbol == NULL || symbol[0] == '\0') {
        symbol = "malloc_size";
    }

    dlerror();
    void* fn = dlsym(RTLD_DEFAULT, symbol);
    const char* err = dlerror();
    if (fn == NULL || err != NULL) {
        fprintf(stderr, "[probe] failed to resolve size symbol=%s: %s\n", symbol, err ? err : "unknown");
        return NULL;
    }

    if (symbol_out != NULL) {
        *symbol_out = symbol;
    }
    return (probe_size_fn_t)fn;
}

static const char* resolve_foreign_mode(void) {
    const char* mode = getenv("HKO_FOREIGN_MODE");
    if (mode == NULL || mode[0] == '\0') {
        return "mmap";
    }
    return mode;
}

static int probe_hakozuna_path(probe_size_fn_t size_fn) {
    const size_t requested = 96;
    void* p = malloc(requested);
    if (!p) {
        fprintf(stderr, "[probe] hakozuna malloc failed\n");
        return 2;
    }

    size_t size = size_fn(p);
    printf("[probe] mode=hakozuna requested=%zu size=%zu\n", requested, size);

    if (size == 0) {
        fprintf(stderr, "[probe] hakozuna malloc_size returned 0\n");
        return 3;
    }
    return 0;
}

static int probe_foreign_path(probe_size_fn_t size_fn) {
    const char* mode = resolve_foreign_mode();
    size_t requested = 0;
    void* p = NULL;

    if (strcmp(mode, "system") == 0) {
        malloc_zone_t* zone = malloc_default_zone();
        if (!zone || !zone->malloc) {
            fprintf(stderr, "[probe] system foreign zone unavailable\n");
            return 4;
        }
        requested = 144;
        p = zone->malloc(zone, requested);
        if (!p) {
            fprintf(stderr, "[probe] foreign system malloc failed\n");
            return 5;
        }
    } else if (strcmp(mode, "mmap") == 0) {
        requested = (size_t)sysconf(_SC_PAGESIZE);
        if (requested == 0) {
            requested = 4096;
        }
        p = mmap(NULL, requested, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (p == MAP_FAILED) {
            fprintf(stderr, "[probe] foreign mmap failed\n");
            return 6;
        }
    } else {
        fprintf(stderr, "[probe] unknown foreign mode=%s\n", mode);
        return 7;
    }

    size_t size = size_fn(p);
    printf("[probe] mode=foreign kind=%s requested=%zu size=%zu\n", mode, requested, size);
    if (strcmp(mode, "mmap") == 0) {
        munmap(p, requested);
    }
    return 0;
}

int main(void) {
    const char* symbol = NULL;
    probe_size_fn_t size_fn = resolve_size_fn(&symbol);
    if (!size_fn) {
        return 1;
    }

    fprintf(stderr, "[probe] size_symbol=%s\n", symbol);

    int rc = probe_hakozuna_path(size_fn);
    if (rc != 0) {
        return rc;
    }

    rc = probe_foreign_path(size_fn);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
