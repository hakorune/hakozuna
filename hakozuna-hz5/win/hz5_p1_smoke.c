#include "hz5.h"

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef struct Hz5SmokeCase {
    size_t size;
    size_t alignment;
} Hz5SmokeCase;

static DWORD WINAPI hz5_smoke_remote_free_thread(LPVOID param) {
    hz5_p2_free(param);
    return 0;
}

int main(void) {
    const Hz5SmokeCase cases[] = {
        {4096u, 8192u},
        {8192u, 8192u},
        {65536u, 8192u},
        {65536u, 65536u}
    };
    void* ptrs[sizeof(cases) / sizeof(cases[0])] = {0};
    int foreign = 0;

    if (hz5_p1_owns(&foreign)) {
        fprintf(stderr, "foreign stack pointer was classified as HZ5-owned\n");
        return 5;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        void* ptr = hz5_p1_alloc_aligned(cases[i].size, cases[i].alignment);
        if (!ptr) {
            fprintf(stderr, "alloc failed: size=%zu align=%zu\n",
                    cases[i].size,
                    cases[i].alignment);
            return 1;
        }
        if (!hz5_p1_validate(ptr, cases[i].size, cases[i].alignment)) {
            fprintf(stderr, "validate failed: ptr=%p size=%zu align=%zu\n",
                    ptr,
                    cases[i].size,
                    cases[i].alignment);
            return 2;
        }
        if (((uintptr_t)ptr % cases[i].alignment) != 0) {
            fprintf(stderr, "alignment failed: ptr=%p size=%zu align=%zu\n",
                    ptr,
                    cases[i].size,
                    cases[i].alignment);
            return 3;
        }
        for (size_t j = 0; j < i; ++j) {
            if (ptr == ptrs[j]) {
                fprintf(stderr, "overlapping live run: ptr=%p\n", ptr);
                return 6;
            }
        }
        ptrs[i] = ptr;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        hz5_p1_free(ptrs[i]);
        if (hz5_p1_validate(ptrs[i], cases[i].size, cases[i].alignment)) {
            fprintf(stderr, "validate unexpectedly passed after free: ptr=%p\n", ptrs[i]);
            return 4;
        }
    }

    void* remote_ptr = hz5_p2_alloc_aligned(8192u, 8192u);
    if (!remote_ptr || !hz5_p1_validate(remote_ptr, 8192u, 8192u)) {
        fprintf(stderr, "P2 owner allocation failed\n");
        return 7;
    }

    HANDLE worker = CreateThread(NULL,
                                 0,
                                 hz5_smoke_remote_free_thread,
                                 remote_ptr,
                                 0,
                                 NULL);
    if (!worker) {
        fprintf(stderr, "CreateThread failed\n");
        return 8;
    }
    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);

    if (!hz5_p1_validate(remote_ptr, 8192u, 8192u)) {
        fprintf(stderr, "remote-free object disappeared before owner drain\n");
        return 9;
    }

    if (hz5_p2_drain_current_owner() != 1) {
        fprintf(stderr, "owner drain did not reclaim exactly one destructor-flushed remote run\n");
        return 10;
    }

    void* remote_reuse = hz5_p2_alloc_aligned(8192u, 8192u);
    if (remote_reuse != remote_ptr) {
        fprintf(stderr, "owner drain did not make remote run reusable: old=%p new=%p\n",
                remote_ptr,
                remote_reuse);
        return 11;
    }
    hz5_p2_free(remote_reuse);

    void* local_ptr = hz5_p2_alloc_aligned(4096u, 8192u);
    if (!local_ptr || !hz5_p1_validate(local_ptr, 4096u, 8192u)) {
        fprintf(stderr, "P2 local allocation failed\n");
        return 12;
    }
    hz5_p2_free(local_ptr);
    void* local_reuse = hz5_p2_alloc_aligned(4096u, 8192u);
    if (local_reuse != local_ptr) {
        fprintf(stderr, "P2 local free did not cache run: old=%p new=%p\n",
                local_ptr,
                local_reuse);
        return 13;
    }
    hz5_p2_free(local_reuse);

    puts("HZ5-P1/P2/P4/P7 aligned-run smoke passed");
    return 0;
}
