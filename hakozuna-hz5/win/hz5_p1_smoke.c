#include "hz5_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define CHECK(cond, err_code, msg, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, msg "\n", ##__VA_ARGS__); \
            return (err_code); \
        } \
    } while(0)

enum Hz5SmokeError {
    ERR_SUCCESS = 0,
    ERR_ALLOC_FAILED = 1,
    ERR_VALIDATE_FAILED = 2,
    ERR_ALIGNMENT_FAILED = 3,
    ERR_VALIDATE_PASSED_AFTER_FREE = 4,
    ERR_FOREIGN_CLASSIFIED_AS_OWNED = 5,
    ERR_OVERLAPPING_LIVE_RUN = 6,
    ERR_P2_OWNER_ALLOC_FAILED = 7,
    ERR_CREATE_THREAD_FAILED = 8,
    ERR_REMOTE_FREE_DISAPPEARED = 9,
    ERR_OWNER_DRAIN_RECLAIM_COUNT = 10,
    ERR_OWNER_DRAIN_REUSE_FAILED = 11,
    ERR_P2_LOCAL_ALLOC_FAILED = 12,
    ERR_P2_LOCAL_FREE_CACHE_FAILED = 13
};

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
    void* ptrs[ARRAY_SIZE(cases)] = {0};
    int foreign = 0;

    CHECK(!hz5_p1_owns(&foreign), ERR_FOREIGN_CLASSIFIED_AS_OWNED, "foreign stack pointer was classified as HZ5-owned");

    for (size_t i = 0; i < ARRAY_SIZE(cases); ++i) {
        void* ptr = hz5_p1_alloc_aligned(cases[i].size, cases[i].alignment);
        CHECK(ptr, ERR_ALLOC_FAILED, "alloc failed: size=%zu align=%zu", cases[i].size, cases[i].alignment);
        CHECK(hz5_p1_validate(ptr, cases[i].size, cases[i].alignment), ERR_VALIDATE_FAILED, "validate failed: ptr=%p size=%zu align=%zu", ptr, cases[i].size, cases[i].alignment);
        CHECK(((uintptr_t)ptr % cases[i].alignment) == 0, ERR_ALIGNMENT_FAILED, "alignment failed: ptr=%p size=%zu align=%zu", ptr, cases[i].size, cases[i].alignment);
        for (size_t j = 0; j < i; ++j) {
            CHECK(ptr != ptrs[j], ERR_OVERLAPPING_LIVE_RUN, "overlapping live run: ptr=%p", ptr);
        }
        ptrs[i] = ptr;
    }

    for (size_t i = 0; i < ARRAY_SIZE(cases); ++i) {
        hz5_p1_free(ptrs[i]);
        CHECK(!hz5_p1_validate(ptrs[i], cases[i].size, cases[i].alignment), ERR_VALIDATE_PASSED_AFTER_FREE, "validate unexpectedly passed after free: ptr=%p", ptrs[i]);
    }

    void* remote_ptr = hz5_p2_alloc_aligned(8192u, 8192u);
    CHECK(remote_ptr && hz5_p1_validate(remote_ptr, 8192u, 8192u), ERR_P2_OWNER_ALLOC_FAILED, "P2 owner allocation failed");

    HANDLE worker = CreateThread(NULL, 0, hz5_smoke_remote_free_thread, remote_ptr, 0, NULL);
    CHECK(worker, ERR_CREATE_THREAD_FAILED, "CreateThread failed");
    WaitForSingleObject(worker, INFINITE);
    CloseHandle(worker);

    CHECK(hz5_p1_validate(remote_ptr, 8192u, 8192u), ERR_REMOTE_FREE_DISAPPEARED, "remote-free object disappeared before owner drain");
    CHECK(hz5_p2_drain_current_owner() == 1, ERR_OWNER_DRAIN_RECLAIM_COUNT, "owner drain did not reclaim exactly one destructor-flushed remote run");

    void* remote_reuse = hz5_p2_alloc_aligned(8192u, 8192u);
    CHECK(remote_reuse == remote_ptr, ERR_OWNER_DRAIN_REUSE_FAILED, "owner drain did not make remote run reusable: old=%p new=%p", remote_ptr, remote_reuse);
    hz5_p2_free(remote_reuse);

    void* local_ptr = hz5_p2_alloc_aligned(4096u, 8192u);
    CHECK(local_ptr && hz5_p1_validate(local_ptr, 4096u, 8192u), ERR_P2_LOCAL_ALLOC_FAILED, "P2 local allocation failed");
    hz5_p2_free(local_ptr);

    void* local_reuse = hz5_p2_alloc_aligned(4096u, 8192u);
    CHECK(local_reuse == local_ptr, ERR_P2_LOCAL_FREE_CACHE_FAILED, "P2 local free did not cache run: old=%p new=%p", local_ptr, local_reuse);
    hz5_p2_free(local_reuse);

    puts("HZ5-P1/P2/P4/P7 aligned-run smoke passed");
    return ERR_SUCCESS;
}
