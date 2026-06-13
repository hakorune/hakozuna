#if HZ3_S65_CENTRAL_COLD_ENABLE && HZ3_S65_CENTRAL_COLD_EXTERNAL_LIST_ENABLE
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_POOL_LAZY
static Hz3CentralColdNode* hz3_central_cold_node_os_alloc(uint32_t count) {
    size_t bytes = (size_t)count * sizeof(Hz3CentralColdNode);
#if defined(_WIN32)
    return (Hz3CentralColdNode*)VirtualAlloc(NULL,
                                             bytes,
                                             MEM_RESERVE | MEM_COMMIT,
                                             PAGE_READWRITE);
#else
    void* p = mmap(NULL,
                   bytes,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1,
                   0);
    if (p == MAP_FAILED) {
        return NULL;
    }
    return (Hz3CentralColdNode*)p;
#endif
}

static int hz3_central_cold_external_pool_grow_locked(void) {
    uint32_t allocated =
        atomic_load_explicit(&g_hz3_cold_node_pool_allocated, memory_order_relaxed);
    if (allocated >= (uint32_t)HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP) {
        return 0;
    }

    uint32_t remaining =
        (uint32_t)HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP - allocated;
    uint32_t count = (uint32_t)HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CHUNK;
    if (count > remaining) {
        count = remaining;
    }

    Hz3CentralColdNode* nodes = hz3_central_cold_node_os_alloc(count);
    if (!nodes) {
        return 0;
    }

    for (uint32_t i = 0; i < count; i++) {
        nodes[i].run = NULL;
        nodes[i].prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
        nodes[i].state = 0;
#endif
        nodes[i].next = g_hz3_cold_free_nodes;
        g_hz3_cold_free_nodes = &nodes[i];
    }

    atomic_store_explicit(&g_hz3_cold_node_pool_allocated,
                          allocated + count,
                          memory_order_relaxed);
    return 1;
}
#endif

static void hz3_central_cold_external_pool_init(void) {
    hz3_lock_init(&g_hz3_cold_node_lock);
    g_hz3_cold_free_nodes = NULL;

#if HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_POOL_LAZY
    atomic_store_explicit(&g_hz3_cold_node_pool_allocated, 0, memory_order_relaxed);
#else
    for (uint32_t i = 0; i < (uint32_t)HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_CAP; i++) {
        g_hz3_cold_nodes[i].run = NULL;
        g_hz3_cold_nodes[i].prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
        g_hz3_cold_nodes[i].state = 0;
#endif
        g_hz3_cold_nodes[i].next = g_hz3_cold_free_nodes;
        g_hz3_cold_free_nodes = &g_hz3_cold_nodes[i];
    }
#endif
}

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
#if !HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
static uint32_t hz3_central_cold_committed_reserve_runs(int sc) {
    uint32_t pages = hz3_sc_to_pages(sc);
    if (pages <= HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES) {
        return (uint32_t)HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_RUNS;
    }
    return (uint32_t)HZ3_S65_CENTRAL_COLD_DECOMMIT_COMMITTED_RESERVE_RUNS;
}

static int hz3_central_cold_should_decommit_locked(Hz3CentralColdExternalBin* bin, int sc) {
    if (hz3_sc_to_pages(sc) < HZ3_S65_CENTRAL_COLD_DECOMMIT_MIN_PAGES) {
        hz3_s65_cold_note_decommit_skip_policy();
        return 0;
    }
    if (bin->ready_count < hz3_central_cold_committed_reserve_runs(sc)) {
        hz3_s65_cold_note_decommit_skip_policy();
        return 0;
    }
    return 1;
}
#endif

static size_t hz3_central_cold_run_bytes(int sc) {
    return (size_t)hz3_sc_to_pages(sc) << HZ3_PAGE_SHIFT;
}

static uint32_t hz3_central_cold_ready_target_runs(int sc) {
#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
    uint32_t pages = hz3_sc_to_pages(sc);
    if (pages >= HZ3_S65_CENTRAL_COLD_LARGE_MIN_PAGES) {
        return (uint32_t)HZ3_S65_CENTRAL_COLD_LARGE_READY_TARGET_RUNS;
    }
    if (pages <= HZ3_S65_CENTRAL_COLD_DECOMMIT_SMALL_RESERVE_MAX_PAGES) {
        return (uint32_t)HZ3_S65_CENTRAL_COLD_SMALL_READY_TARGET_RUNS;
    }
    return (uint32_t)HZ3_S65_CENTRAL_COLD_READY_TARGET_RUNS;
#else
    return hz3_central_cold_committed_reserve_runs(sc);
#endif
}

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
static int hz3_central_cold_decommit_coalesce_enabled_for_sc(int sc) {
    uint32_t pages = hz3_sc_to_pages(sc);
    return pages >= HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_MIN_PAGES &&
           pages <= HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_MAX_PAGES;
}
#endif

static int hz3_central_cold_node_is_decommitted(const Hz3CentralColdNode* node) {
    return node && node->state == HZ3_CENTRAL_COLD_STATE_DECOMMITTED;
}

static void hz3_central_cold_note_state_or_mismatch(Hz3CentralColdNode* node,
                                                    Hz3CentralColdState expected) {
    if (!node || node->state != (uint8_t)expected) {
        hz3_s65_cold_note_state_mismatch();
    }
}

#if defined(_WIN32)
static hz3_once_t g_hz3_cold_qpc_once = HZ3_ONCE_INIT;
static int64_t g_hz3_cold_qpc_freq = 0;

static void hz3_central_cold_qpc_init(void) {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
        g_hz3_cold_qpc_freq = freq.QuadPart;
    }
}
#endif

static uint64_t hz3_central_cold_now_ns(void) {
#if defined(_WIN32)
    hz3_once(&g_hz3_cold_qpc_once, hz3_central_cold_qpc_init);
    if (g_hz3_cold_qpc_freq <= 0) {
        return 0;
    }
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }
    return (uint64_t)(((double)counter.QuadPart * 1000000000.0) /
                      (double)g_hz3_cold_qpc_freq);
#else
    return 0;
#endif
}

static int hz3_central_cold_decommit_run(void* run, size_t bytes) {
    if (!run || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    int ok = VirtualFree(run, bytes, MEM_DECOMMIT) ? 1 : 0;
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_decommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_decommit(1, bytes, 0);
    return 0;
#endif
}

static int hz3_central_cold_recommit_run(void* run, size_t bytes) {
    if (!run || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    void* p = VirtualAlloc(run, bytes, MEM_COMMIT, PAGE_READWRITE);
    int ok = (p == run);
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_recommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_recommit(1, bytes, 0);
    return 0;
#endif
}

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE
typedef struct Hz3CentralColdDecommitObserveBatch {
    Hz3CentralColdNode* nodes[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS];
    uint32_t count;
} Hz3CentralColdDecommitObserveBatch;

static void hz3_central_cold_decommit_observe_sort(Hz3CentralColdNode** nodes,
                                                   uint32_t n) {
    for (uint32_t i = 1; i < n; i++) {
        Hz3CentralColdNode* cur = nodes[i];
        uintptr_t cur_addr = (uintptr_t)cur->run;
        uint32_t j = i;
        while (j > 0 && (uintptr_t)nodes[j - 1]->run > cur_addr) {
            nodes[j] = nodes[j - 1];
            j--;
        }
        nodes[j] = cur;
    }
}

static void hz3_central_cold_decommit_observe_flush(
    Hz3CentralColdDecommitObserveBatch* batch,
    int sc) {
    if (!batch || batch->count == 0) {
        return;
    }

    Hz3CentralColdNode* sorted[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS];
    uint32_t n = batch->count;
    for (uint32_t i = 0; i < n; i++) {
        sorted[i] = batch->nodes[i];
    }
    batch->count = 0;

    hz3_central_cold_decommit_observe_sort(sorted, n);

    size_t run_bytes = hz3_central_cold_run_bytes(sc);
    uint64_t ranges = 0;
    uint64_t saved_syscalls = 0;
    size_t max_range_bytes = 0;
    uint32_t i = 0;
    while (i < n) {
        uintptr_t base = (uintptr_t)sorted[i]->run;
        uintptr_t end = base + run_bytes;
        uint32_t range_nodes = 1;
        i++;
        while (i < n && (uintptr_t)sorted[i]->run == end) {
            end += run_bytes;
            range_nodes++;
            i++;
        }

        size_t range_bytes = (size_t)(end - base);
        if (range_bytes > max_range_bytes) {
            max_range_bytes = range_bytes;
        }
        ranges++;
        saved_syscalls += (uint64_t)(range_nodes - 1);
    }

    hz3_s65_cold_note_decommit_coalesce_observe(n,
                                                ranges,
                                                saved_syscalls,
                                                (uint64_t)max_range_bytes);
}

static void hz3_central_cold_decommit_observe_note(
    Hz3CentralColdDecommitObserveBatch* batch,
    Hz3CentralColdNode* node,
    int sc) {
    if (!batch || !node || !node->run) {
        return;
    }
    batch->nodes[batch->count++] = node;
    if (batch->count >= HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_OBSERVE_BATCH_RUNS) {
        hz3_central_cold_decommit_observe_flush(batch, sc);
    }
}
#define HZ3_COLD_DECOMMIT_OBS_DECL(name) \
    Hz3CentralColdDecommitObserveBatch name = { { 0 }, 0 }
#define HZ3_COLD_DECOMMIT_OBS_NOTE(name, node, sc) \
    hz3_central_cold_decommit_observe_note(&(name), (node), (sc))
#define HZ3_COLD_DECOMMIT_OBS_FLUSH(name, sc) \
    hz3_central_cold_decommit_observe_flush(&(name), (sc))
#else
#define HZ3_COLD_DECOMMIT_OBS_DECL(name) do { } while (0)
#define HZ3_COLD_DECOMMIT_OBS_NOTE(name, node, sc) do { (void)(node); } while (0)
#define HZ3_COLD_DECOMMIT_OBS_FLUSH(name, sc) do { } while (0)
#endif

#if HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_ENABLE
static int hz3_central_cold_decommit_range(void* base, size_t bytes) {
    if (!base || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    int ok = VirtualFree(base, bytes, MEM_DECOMMIT) ? 1 : 0;
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_decommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_decommit(1, bytes, 0);
    return 0;
#endif
}

static void hz3_central_cold_decommit_sort_nodes(Hz3CentralColdNode** nodes,
                                                 uint32_t n) {
    for (uint32_t i = 1; i < n; i++) {
        Hz3CentralColdNode* cur = nodes[i];
        uintptr_t cur_addr = (uintptr_t)cur->run;
        uint32_t j = i;
        while (j > 0 && (uintptr_t)nodes[j - 1]->run > cur_addr) {
            nodes[j] = nodes[j - 1];
            j--;
        }
        nodes[j] = cur;
    }
}

static void hz3_central_cold_mark_node_decommit_ok(Hz3CentralColdNode** nodes,
                                                   uint8_t* decommit_ok,
                                                   uint32_t n,
                                                   Hz3CentralColdNode* target,
                                                   int ok) {
    for (uint32_t i = 0; i < n; i++) {
        if (nodes[i] == target) {
            decommit_ok[i] = (uint8_t)ok;
            return;
        }
    }
}

static void hz3_central_cold_decommit_nodes_coalesced(Hz3CentralColdNode** nodes,
                                                      uint8_t* decommit_ok,
                                                      uint32_t n,
                                                      int sc) {
    if (n == 0) {
        return;
    }

    Hz3CentralColdNode* sorted[HZ3_S65_CENTRAL_COLD_DECOMMIT_COALESCE_BATCH_RUNS];
    for (uint32_t i = 0; i < n; i++) {
        sorted[i] = nodes[i];
        decommit_ok[i] = 0;
    }
    hz3_central_cold_decommit_sort_nodes(sorted, n);

    size_t run_bytes = hz3_central_cold_run_bytes(sc);
    uint32_t ranges = 0;
    uint64_t saved_syscalls = 0;
    size_t max_range_bytes = 0;
    uint32_t i = 0;

    while (i < n) {
        uint32_t start = i;
        uintptr_t base = (uintptr_t)sorted[i]->run;
        uintptr_t end = base + run_bytes;
        i++;

        while (i < n && (uintptr_t)sorted[i]->run == end) {
            end += run_bytes;
            i++;
        }

        size_t range_bytes = (size_t)(end - base);
        if (range_bytes > max_range_bytes) {
            max_range_bytes = range_bytes;
        }
        ranges++;

        int ok = hz3_central_cold_decommit_range((void*)base, range_bytes) == 0;
        if (ok) {
            for (uint32_t j = start; j < i; j++) {
                hz3_central_cold_mark_node_decommit_ok(nodes,
                                                       decommit_ok,
                                                       n,
                                                       sorted[j],
                                                       1);
            }
            saved_syscalls += (uint64_t)((i - start) - 1);
            continue;
        }

        // A range may cross a reservation boundary. Fall back to individual
        // decommit so the experiment stays conservative and rollbackable.
        for (uint32_t j = start; j < i; j++) {
            Hz3CentralColdNode* node = sorted[j];
            int single_ok =
                hz3_central_cold_decommit_run(node->run, run_bytes) == 0;
            hz3_central_cold_mark_node_decommit_ok(nodes,
                                                   decommit_ok,
                                                   n,
                                                   node,
                                                   single_ok);
        }
    }

    hz3_s65_cold_note_decommit_coalesce_observe(n,
                                                ranges,
                                                saved_syscalls,
                                                (uint64_t)max_range_bytes);
}
#endif

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
static int hz3_central_cold_recommit_range(void* base, size_t bytes) {
    if (!base || bytes == 0) {
        return -1;
    }
    uint64_t start_ns = hz3_central_cold_now_ns();
#if defined(_WIN32)
    void* p = VirtualAlloc(base, bytes, MEM_COMMIT, PAGE_READWRITE);
    int ok = (p == base);
    uint64_t end_ns = hz3_central_cold_now_ns();
    hz3_s65_cold_note_recommit(ok,
                                bytes,
                                end_ns >= start_ns ? end_ns - start_ns : 0);
    return ok ? 0 : -1;
#else
    hz3_s65_cold_note_recommit(1, bytes, 0);
    return 0;
#endif
}
#endif

static void hz3_central_cold_ready_push_head_locked(Hz3CentralColdExternalBin* bin,
                                                    Hz3CentralColdNode* node) {
    node->state = HZ3_CENTRAL_COLD_STATE_READY;
    node->prev = NULL;
    node->next = bin->ready_head;
    if (bin->ready_head) {
        bin->ready_head->prev = node;
    } else {
        bin->ready_tail = node;
    }
    bin->ready_head = node;
    bin->ready_count++;
}

static Hz3CentralColdNode* hz3_central_cold_ready_pop_head_locked(
    Hz3CentralColdExternalBin* bin) {
    Hz3CentralColdNode* node = bin->ready_head;
    if (!node) {
        return NULL;
    }
    hz3_central_cold_note_state_or_mismatch(node, HZ3_CENTRAL_COLD_STATE_READY);
    bin->ready_head = node->next;
    if (bin->ready_head) {
        bin->ready_head->prev = NULL;
    } else {
        bin->ready_tail = NULL;
    }
    node->next = NULL;
    node->prev = NULL;
    if (bin->ready_count > 0) {
        bin->ready_count--;
    }
    return node;
}

static Hz3CentralColdNode* hz3_central_cold_ready_pop_tail_locked(
    Hz3CentralColdExternalBin* bin) {
    Hz3CentralColdNode* node = bin->ready_tail;
    if (!node) {
        return NULL;
    }
    hz3_central_cold_note_state_or_mismatch(node, HZ3_CENTRAL_COLD_STATE_READY);
    bin->ready_tail = node->prev;
    if (bin->ready_tail) {
        bin->ready_tail->next = NULL;
    } else {
        bin->ready_head = NULL;
    }
    node->next = NULL;
    node->prev = NULL;
    if (bin->ready_count > 0) {
        bin->ready_count--;
    }
    return node;
}

static void hz3_central_cold_decommitted_push_head_locked(
    Hz3CentralColdExternalBin* bin,
    Hz3CentralColdNode* node) {
    node->state = HZ3_CENTRAL_COLD_STATE_DECOMMITTED;
    node->prev = NULL;
    node->next = bin->decommitted_head;
    bin->decommitted_head = node;
    bin->decommitted_count++;
}

static Hz3CentralColdNode* hz3_central_cold_decommitted_pop_head_locked(
    Hz3CentralColdExternalBin* bin) {
    Hz3CentralColdNode* node = bin->decommitted_head;
    if (!node) {
        return NULL;
    }
    hz3_central_cold_note_state_or_mismatch(node, HZ3_CENTRAL_COLD_STATE_DECOMMITTED);
    bin->decommitted_head = node->next;
    node->next = NULL;
    node->prev = NULL;
    if (bin->decommitted_count > 0) {
        bin->decommitted_count--;
    }
    return node;
}

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_BATCH_RUNS > 1
static uint32_t hz3_central_cold_rewarm_decommitted_locked(Hz3CentralColdExternalBin* bin,
                                                           int sc,
                                                           uint32_t limit) {
    uint32_t warmed = 0;
    while (warmed < limit && bin->decommitted_head) {
        Hz3CentralColdNode* node = hz3_central_cold_decommitted_pop_head_locked(bin);
        if (!node) {
            break;
        }
        if (hz3_central_cold_node_is_decommitted(node) &&
            hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) != 0) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] rewarm recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
        hz3_central_cold_ready_push_head_locked(bin, node);
        warmed++;
    }

    if (warmed > 0) {
        hz3_s65_cold_note_decommitted_pop(sc, warmed);
        hz3_s65_cold_note_committed_push(sc, warmed);
        hz3_s65_cold_note_rewarm(warmed);
    }
    return warmed;
}

#if HZ3_S65_CENTRAL_COLD_READY_MRU_ENABLE
enum { HZ3_CENTRAL_COLD_REWARM_DETACH_MAX = 16 };

#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
static void hz3_central_cold_sort_nodes_by_run(Hz3CentralColdNode** nodes,
                                               uint32_t n) {
    for (uint32_t i = 1; i < n; i++) {
        Hz3CentralColdNode* cur = nodes[i];
        uintptr_t cur_addr = (uintptr_t)cur->run;
        uint32_t j = i;
        while (j > 0 && (uintptr_t)nodes[j - 1]->run > cur_addr) {
            nodes[j] = nodes[j - 1];
            j--;
        }
        nodes[j] = cur;
    }
}

static void hz3_central_cold_mark_node_commit_ok(Hz3CentralColdNode** nodes,
                                                 uint8_t* commit_ok,
                                                 uint32_t n,
                                                 Hz3CentralColdNode* target,
                                                 int ok) {
    for (uint32_t i = 0; i < n; i++) {
        if (nodes[i] == target) {
            commit_ok[i] = (uint8_t)ok;
            return;
        }
    }
}

static void hz3_central_cold_recommit_nodes_coalesced(Hz3CentralColdNode** nodes,
                                                      uint8_t* commit_ok,
                                                      uint32_t n,
                                                      int sc) {
    if (n == 0) {
        return;
    }

    Hz3CentralColdNode* sorted[HZ3_CENTRAL_COLD_REWARM_DETACH_MAX];
    for (uint32_t i = 0; i < n; i++) {
        sorted[i] = nodes[i];
    }
    hz3_central_cold_sort_nodes_by_run(sorted, n);

    size_t run_bytes = hz3_central_cold_run_bytes(sc);
    uint32_t ranges = 0;
    uint64_t saved_syscalls = 0;
    size_t max_range_bytes = 0;
    uint32_t i = 0;

    while (i < n) {
        uint32_t start = i;
        uintptr_t base = (uintptr_t)sorted[i]->run;
        uintptr_t end = base + run_bytes;
        i++;

        while (i < n && (uintptr_t)sorted[i]->run == end) {
            end += run_bytes;
            i++;
        }

        size_t range_bytes = (size_t)(end - base);
        if (range_bytes > max_range_bytes) {
            max_range_bytes = range_bytes;
        }
        ranges++;

        int ok = hz3_central_cold_recommit_range((void*)base, range_bytes) == 0;
        if (ok) {
            for (uint32_t j = start; j < i; j++) {
                hz3_central_cold_mark_node_commit_ok(nodes,
                                                     commit_ok,
                                                     n,
                                                     sorted[j],
                                                     1);
            }
            saved_syscalls += (uint64_t)((i - start) - 1);
            continue;
        }

        // Range commit can fail if adjacent runs cross separate reservations.
        // Fall back to individual commits so the coalesced lane stays safe.
        for (uint32_t j = start; j < i; j++) {
            Hz3CentralColdNode* node = sorted[j];
            int single_ok =
                hz3_central_cold_recommit_run(node->run, run_bytes) == 0;
            if (!single_ok && HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST) {
                fprintf(stderr,
                        "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] coalesced fallback recommit failed run=%p sc=%d bytes=%zu\n",
                        node->run,
                        sc,
                        run_bytes);
                abort();
            }
            hz3_central_cold_mark_node_commit_ok(nodes,
                                                 commit_ok,
                                                 n,
                                                 node,
                                                 single_ok);
        }
    }

    hz3_s65_cold_note_recommit_coalesce(n,
                                        ranges,
                                        saved_syscalls,
                                        (uint64_t)max_range_bytes);
}
#endif

static uint32_t hz3_central_cold_rewarm_decommitted_outside_lock(
    Hz3CentralColdExternalBin* bin,
    int sc,
    uint32_t limit) {
    Hz3CentralColdNode* nodes[HZ3_CENTRAL_COLD_REWARM_DETACH_MAX];
    uint8_t commit_ok[HZ3_CENTRAL_COLD_REWARM_DETACH_MAX];
    uint32_t detached = 0;

    if (limit > HZ3_CENTRAL_COLD_REWARM_DETACH_MAX) {
        limit = HZ3_CENTRAL_COLD_REWARM_DETACH_MAX;
    }

    hz3_lock_acquire(&bin->lock);
    while (detached < limit && bin->decommitted_head) {
        Hz3CentralColdNode* node = hz3_central_cold_decommitted_pop_head_locked(bin);
        if (!node) {
            break;
        }
        node->state = HZ3_CENTRAL_COLD_STATE_RECOMMITTING;
        bin->inflight_recommitting++;
        nodes[detached] = node;
        commit_ok[detached] = 0;
        detached++;
    }
    hz3_lock_release(&bin->lock);

    if (detached == 0) {
        return 0;
    }

    hz3_s65_cold_note_inflight_recommitting_add(detached);
#if HZ3_S65_CENTRAL_COLD_RECOMMIT_COALESCE_ENABLE
    hz3_central_cold_recommit_nodes_coalesced(nodes, commit_ok, detached, sc);
#else
    for (uint32_t i = 0; i < detached; i++) {
        Hz3CentralColdNode* node = nodes[i];
        int ok = hz3_central_cold_recommit_run(node->run, hz3_central_cold_run_bytes(sc)) == 0;
        if (!ok && HZ3_S65_CENTRAL_COLD_RECOMMIT_FAILFAST) {
            fprintf(stderr,
                    "[HZ3_S65_CENTRAL_COLD_DECOMMIT_FAILFAST] mru rewarm recommit failed run=%p sc=%d bytes=%zu\n",
                    node->run,
                    sc,
                    hz3_central_cold_run_bytes(sc));
            abort();
        }
        commit_ok[i] = (uint8_t)ok;
    }
#endif

    uint32_t warmed = 0;
    uint32_t failed = 0;
    hz3_lock_acquire(&bin->lock);
    for (uint32_t i = 0; i < detached; i++) {
        Hz3CentralColdNode* node = nodes[i];
        if (bin->inflight_recommitting > 0) {
            bin->inflight_recommitting--;
        }
        if (commit_ok[i]) {
            hz3_central_cold_ready_push_head_locked(bin, node);
            warmed++;
        } else {
            hz3_central_cold_decommitted_push_head_locked(bin, node);
            failed++;
        }
    }
    hz3_lock_release(&bin->lock);
    hz3_s65_cold_note_inflight_recommitting_sub(detached);

    hz3_s65_cold_note_decommitted_pop(sc, detached);
    hz3_s65_cold_note_unlock_recommit(detached);
    if (warmed > 0) {
        hz3_s65_cold_note_committed_push(sc, warmed);
        hz3_s65_cold_note_rewarm(warmed);
    }
    if (failed > 0) {
        hz3_s65_cold_note_decommitted_push(sc, failed);
    }
    return warmed;
}
#endif
#endif
#endif

static Hz3CentralColdNode* hz3_central_cold_external_node_alloc(void* run) {
    Hz3CentralColdNode* node = NULL;

    hz3_lock_acquire(&g_hz3_cold_node_lock);
    node = g_hz3_cold_free_nodes;
#if HZ3_S65_CENTRAL_COLD_EXTERNAL_NODE_POOL_LAZY
    if (!node && hz3_central_cold_external_pool_grow_locked()) {
        node = g_hz3_cold_free_nodes;
    }
#endif
    if (node) {
        g_hz3_cold_free_nodes = node->next;
    }
    hz3_lock_release(&g_hz3_cold_node_lock);

    if (!node) {
        hz3_s65_cold_ext_note_alloc_fail();
        return NULL;
    }

    node->run = run;
    node->next = NULL;
    node->prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    node->state = HZ3_CENTRAL_COLD_STATE_READY;
#endif
    hz3_s65_cold_ext_note_node_alloc();
    return node;
}

static void hz3_central_cold_external_node_free(Hz3CentralColdNode* node) {
    if (!node) {
        return;
    }

    node->run = NULL;
    node->prev = NULL;
#if HZ3_S65_CENTRAL_COLD_DECOMMIT_ENABLE
    node->state = 0;
#endif
    hz3_lock_acquire(&g_hz3_cold_node_lock);
    node->next = g_hz3_cold_free_nodes;
    g_hz3_cold_free_nodes = node;
    hz3_lock_release(&g_hz3_cold_node_lock);
    hz3_s65_cold_ext_note_node_free();
}
#endif
