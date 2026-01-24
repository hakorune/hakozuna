#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

// Longer MT stress test for hz3 Day 5 GO/NO-GO
// Target: ~30 seconds with T=4

#define NUM_THREADS 4
#define DURATION_SECS 30
#define MAX_LIVE 256

static _Atomic uint64_t g_total_allocs = 0;
static _Atomic uint64_t g_total_frees = 0;
static _Atomic int g_running = 1;

// Cross-thread exchange for remote free testing
static void* volatile g_exchange[NUM_THREADS];
static pthread_mutex_t g_exchange_lock = PTHREAD_MUTEX_INITIALIZER;

static void* thread_func(void* arg) {
    int tid = (int)(long)arg;
    void* live[MAX_LIVE] = {0};
    int live_count = 0;
    unsigned int seed = (unsigned int)tid + 1;
    uint64_t local_allocs = 0, local_frees = 0;
    int iter = 0;

    while (atomic_load(&g_running)) {
        // 50% alloc, 50% free (when we have live objects)
        int do_alloc = (live_count == 0) || (rand_r(&seed) % 2 == 0 && live_count < MAX_LIVE);

        if (do_alloc) {
            // Size class 0-7 (4KB-32KB)
            int sc = rand_r(&seed) % 8;
            size_t size = (size_t)(sc + 1) * 4096;
            void* p = malloc(size);
            if (p) {
                *(volatile char*)p = (char)tid;
                live[live_count++] = p;
                local_allocs++;
            }
        } else if (live_count > 0) {
            int idx = rand_r(&seed) % live_count;
            void* p = live[idx];

            // 10% chance: cross-thread exchange (remote free)
            if (rand_r(&seed) % 10 == 0) {
                int target = rand_r(&seed) % NUM_THREADS;
                pthread_mutex_lock(&g_exchange_lock);
                void* old = g_exchange[target];
                g_exchange[target] = p;
                p = old;
                pthread_mutex_unlock(&g_exchange_lock);
            }

            if (p) {
                free(p);
                local_frees++;
            }

            live[idx] = live[--live_count];
        }

        iter++;
        // Pick up cross-thread objects periodically
        if (iter % 100000 == 0) {
            pthread_mutex_lock(&g_exchange_lock);
            void* p = g_exchange[tid];
            g_exchange[tid] = NULL;
            pthread_mutex_unlock(&g_exchange_lock);
            if (p) {
                free(p);
                local_frees++;
            }
        }
    }

    // Cleanup remaining live objects
    for (int i = 0; i < live_count; i++) {
        free(live[i]);
        local_frees++;
    }

    atomic_fetch_add(&g_total_allocs, local_allocs);
    atomic_fetch_add(&g_total_frees, local_frees);
    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    struct timespec start, end;

    printf("hz3 MT stress test (long): T=%d, duration=%ds\n", NUM_THREADS, DURATION_SECS);
    printf("Starting...\n");
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, (void*)(long)i);
    }

    sleep(DURATION_SECS);
    atomic_store(&g_running, 0);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Cleanup any remaining exchange objects
    for (int i = 0; i < NUM_THREADS; i++) {
        if (g_exchange[i]) {
            free(g_exchange[i]);
            atomic_fetch_add(&g_total_frees, 1);
        }
    }

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    uint64_t total_allocs = atomic_load(&g_total_allocs);
    uint64_t total_frees = atomic_load(&g_total_frees);

    printf("\n=== Results ===\n");
    printf("Elapsed: %.2f seconds\n", elapsed);
    printf("Total allocs: %lu\n", total_allocs);
    printf("Total frees:  %lu\n", total_frees);
    printf("Ops/sec: %.2fM\n", (total_allocs + total_frees) / elapsed / 1e6);

    if (total_allocs == total_frees) {
        printf("\nPASS: alloc/free balanced, no crash\n");
        return 0;
    } else {
        printf("\nFAIL: alloc/free mismatch (diff=%ld)\n", (long)(total_allocs - total_frees));
        return 1;
    }
}
