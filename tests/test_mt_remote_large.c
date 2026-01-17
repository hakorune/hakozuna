// hz3 Day 4 MT remote free test - larger scale, multiple sizes
// T=4 threads, each allocates and the next thread frees

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#define NUM_THREADS 4
#define ALLOCS_PER_THREAD 5000
#define NUM_SIZES 8

// Size classes: 4KB, 8KB, 12KB, 16KB, 20KB, 24KB, 28KB, 32KB
static const size_t SIZES[NUM_SIZES] = {
    4096, 8192, 12288, 16384, 20480, 24576, 28672, 32768
};

// Shared array: thread i puts pointers here, thread (i+1)%T frees them
static void* shared_ptrs[NUM_THREADS][ALLOCS_PER_THREAD];
static size_t shared_sizes[NUM_THREADS][ALLOCS_PER_THREAD];
static pthread_barrier_t barrier;

typedef struct {
    int thread_id;
} thread_arg_t;

static void* thread_func(void* arg) {
    thread_arg_t* ta = (thread_arg_t*)arg;
    int tid = ta->thread_id;
    int next_tid = (tid + 1) % NUM_THREADS;

    // Phase 1: Allocate and store in shared array (mixed sizes)
    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        size_t size = SIZES[i % NUM_SIZES];
        void* ptr = malloc(size);
        if (!ptr) {
            fprintf(stderr, "T%d: malloc(%zu) failed at i=%d\n", tid, size, i);
            return (void*)1;
        }
        // Touch memory to ensure it's mapped
        memset(ptr, tid, size);
        shared_ptrs[tid][i] = ptr;
        shared_sizes[tid][i] = size;
    }

    // Barrier: wait for all threads to finish allocation
    pthread_barrier_wait(&barrier);

    // Phase 2: Free the next thread's allocations (remote free)
    for (int i = 0; i < ALLOCS_PER_THREAD; i++) {
        void* ptr = shared_ptrs[next_tid][i];
        if (ptr) {
            // Verify memory wasn't corrupted
            uint8_t* bytes = (uint8_t*)ptr;
            if (bytes[0] != (uint8_t)next_tid) {
                fprintf(stderr, "T%d: memory corruption at ptr from T%d\n", tid, next_tid);
                return (void*)2;
            }
            free(ptr);
        }
    }

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];

    // Initialize barrier
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    // Clear shared array
    memset(shared_ptrs, 0, sizeof(shared_ptrs));
    memset(shared_sizes, 0, sizeof(shared_sizes));

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i;
        if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }

    // Join threads
    int failed = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        void* retval;
        pthread_join(threads[i], &retval);
        if (retval != NULL) {
            fprintf(stderr, "Thread %d failed with code %ld\n", i, (long)retval);
            failed = 1;
        }
    }

    pthread_barrier_destroy(&barrier);

    if (failed) {
        printf("FAIL: MT remote free test failed\n");
        return 1;
    }

    printf("OK: MT remote free (large) passed (T=%d, allocs=%d each, %d sizes)\n",
           NUM_THREADS, ALLOCS_PER_THREAD, NUM_SIZES);
    return 0;
}
