#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

// Simple MT stress test for hz3 Day 5
// Tests: alloc/free with local and remote patterns

#define NUM_THREADS 4
#define ITERS_PER_THREAD 2500000
#define MAX_LIVE 256

static _Atomic uint64_t g_total_allocs = 0;
static _Atomic uint64_t g_total_frees = 0;
static _Atomic int g_thread_done[NUM_THREADS];

// Cross-thread exchange for remote free testing
static void* volatile g_exchange[NUM_THREADS];
static pthread_mutex_t g_exchange_lock = PTHREAD_MUTEX_INITIALIZER;

static void* thread_func(void* arg) {
    int tid = (int)(long)arg;
    void* live[MAX_LIVE] = {0};
    int live_count = 0;
    unsigned int seed = (unsigned int)tid + 1;

    for (int i = 0; i < ITERS_PER_THREAD; i++) {
        // 50% alloc, 50% free (when we have live objects)
        int do_alloc = (live_count == 0) || (rand_r(&seed) % 2 == 0 && live_count < MAX_LIVE);

        if (do_alloc) {
            // Size class 0-7 (4KB-32KB)
            int sc = rand_r(&seed) % 8;
            size_t size = (size_t)(sc + 1) * 4096;
            void* p = malloc(size);
            if (p) {
                // Touch memory to ensure it's valid
                *(volatile char*)p = (char)tid;
                live[live_count++] = p;
                atomic_fetch_add(&g_total_allocs, 1);
            }
        } else if (live_count > 0) {
            // Free a random live object
            int idx = rand_r(&seed) % live_count;
            void* p = live[idx];

            // 10% chance: cross-thread exchange (remote free simulation)
            if (rand_r(&seed) % 10 == 0) {
                int target = rand_r(&seed) % NUM_THREADS;
                pthread_mutex_lock(&g_exchange_lock);
                void* old = g_exchange[target];
                g_exchange[target] = p;
                p = old;  // Take the one that was there (if any)
                pthread_mutex_unlock(&g_exchange_lock);
            }

            if (p) {
                free(p);
                atomic_fetch_add(&g_total_frees, 1);
            }

            // Remove from live array
            live[idx] = live[--live_count];
        }

        // Every 100K iters, try to pick up cross-thread objects
        if (i % 100000 == 0 && i > 0) {
            pthread_mutex_lock(&g_exchange_lock);
            void* p = g_exchange[tid];
            g_exchange[tid] = NULL;
            pthread_mutex_unlock(&g_exchange_lock);
            if (p) {
                free(p);
                atomic_fetch_add(&g_total_frees, 1);
            }
        }
    }

    // Cleanup remaining live objects
    for (int i = 0; i < live_count; i++) {
        free(live[i]);
        atomic_fetch_add(&g_total_frees, 1);
    }

    atomic_store(&g_thread_done[tid], 1);
    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    struct timespec start, end;

    printf("hz3 MT stress test: T=%d, iters=%d per thread\n", NUM_THREADS, ITERS_PER_THREAD);
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_func, (void*)(long)i);
    }

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

    printf("Completed in %.2f seconds\n", elapsed);
    printf("Total allocs: %lu\n", total_allocs);
    printf("Total frees:  %lu\n", total_frees);
    printf("Ops/sec: %.2fM\n", (total_allocs + total_frees) / elapsed / 1e6);

    if (total_allocs == total_frees) {
        printf("PASS: alloc/free balanced\n");
        return 0;
    } else {
        printf("FAIL: alloc/free mismatch (diff=%ld)\n", (long)(total_allocs - total_frees));
        return 1;
    }
}
