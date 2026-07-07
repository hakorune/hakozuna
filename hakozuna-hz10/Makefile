.PHONY: all clean smoke-pagemap-route smoke-pagemap-route-diff \
	bench-pagemap-route \
	smoke-freelist-page bench-freelist-page \
	smoke-remote-stack-drain bench-remote-stack-drain bench-multiclass-remote \
	bench-remote-rmw-micro \
	smoke-bounded-page-pool bench-bounded-page-pool smoke-class-pages \
	smoke-size-class \
	smoke-public-entry smoke-public-entry-owner \
	smoke-public-entry-orphan-adoption smoke-public-entry-orphan-partial \
	smoke-public-entry-orphan-partial-front \
	smoke-public-entry-owner-orphan-partial-front \
	bench-public-entry \
	bench-public-entry-active-scan \
	bench-public-entry-active-mtf bench-public-entry-two-slot \
	bench-public-entry-remote-batch-locality bench-public-entry-stage-cost \
	bench-public-entry-local-path \
	smoke-public-entry-retired-local bench-public-entry-retired-local \
	smoke-public-entry-front bench-public-entry-front \
	bench-public-entry-local-path-front smoke-public-entry-front-array \
	bench-public-entry-front-array bench-public-entry-local-path-front-array \
	bench-class-pages-scan \
	smoke-retired-ready bench-retired-ready bench-public-entry-steady-state \
	bench-public-entry-thread-reuse \
	preload preload-bump preload-base preload-fine preload-front preload-coarse \
	preload-orphan-adoption preload-orphan-partial preload-thread-stats \
	preload-fine-size-classes preload-retired-local \
	preload-fine-retired-local smoke-shim-api smoke-shim-foreign \
	bench-macro-preload bench-macro-matrix \
	bench-larson-thread-churn-attribution bench-hz8-public-purge-matrix \
	hz10-rss-guard hz10-standalone-check smoke-tsan-aslr-off

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
LDFLAGS ?=
LDLIBS ?= -pthread
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src
DEBUG_CFLAGS := $(CFLAGS) -DHZ10_ENABLE_DEBUG_STATS
# HZ10EntryTrim Step 0: LTO is part of the optimized allocator/bench lane.
# Keep DEBUG_CFLAGS separate so smoke/sanitizer/TSan-style compiler modes stay
# explicit gates instead of inheriting optimizer-mode assumptions.
OPT_CFLAGS := $(CFLAGS) -flto
# -ftls-model=initial-exec: safe and cheap specifically for a preloaded
# .so -- LD_PRELOAD libraries load at process start and are never dlopen'd
# or unloaded later, satisfying initial-exec's requirement. Perf profiling
# on sh6bench (bench_results/20260707T080000Z_hz10_sh6bench_attribution)
# found __tls_get_addr plus owner-lookup wrapper calls eating ~20%+ of
# self time under the shim's default general-dynamic TLS model (a cost
# that never shows up in the statically-linked micro benches, since those
# already get local-exec for free) -- measured +13-15% wall on that row
# with zero RSS or correctness change.
SHIM_CFLAGS := $(OPT_CFLAGS) -fno-builtin -ftls-model=initial-exec
# Bind calls to functions defined inside libhz10*.so locally. This is scoped to
# the preload artifacts only: host-program malloc/free interposition remains
# exported, while internal hz10_malloc/hz10_free/page helper calls avoid PLT
# indirection. See HZ10ShimInternalBinding-L0.
SHIM_LDFLAGS := $(LDFLAGS) -Wl,-Bsymbolic-functions
TSAN_DEBUG_CFLAGS := $(CFLAGS) -DHZ10_ENABLE_DEBUG_STATS -fsanitize=thread -fno-omit-frame-pointer
TSAN_LDFLAGS := $(LDFLAGS) -fsanitize=thread

HEADERS := $(wildcard $(ROOT)/src/*.h) $(wildcard $(ROOT)/include/*.h)
PAGEMAP_SRC := $(ROOT)/src/hz10_pagemap.c $(ROOT)/src/hz10_platform.c
SMOKE_PAGEMAP_SRC := $(ROOT)/tests/hz10_pagemap_route_smoke.c $(PAGEMAP_SRC)
SMOKE_PAGEMAP_DIFF_SRC := $(ROOT)/tests/hz10_pagemap_route_diff_smoke.c $(PAGEMAP_SRC)
BENCH_PAGEMAP_SRC := $(ROOT)/bench/hz10_pagemap_route_bench.c $(PAGEMAP_SRC)

FREELIST_SRC := $(ROOT)/src/hz10_freelist_page.c $(PAGEMAP_SRC)
SMOKE_FREELIST_SRC := $(ROOT)/tests/hz10_freelist_page_smoke.c $(FREELIST_SRC)
BENCH_FREELIST_SRC := $(ROOT)/bench/hz10_freelist_page_bench.c $(FREELIST_SRC)

REMOTE_STACK_SRC := $(ROOT)/src/hz10_remote_stack.c $(FREELIST_SRC)
SMOKE_REMOTE_STACK_SRC := $(ROOT)/tests/hz10_remote_stack_drain_smoke.c $(REMOTE_STACK_SRC)
BENCH_REMOTE_STACK_SRC := $(ROOT)/bench/hz10_remote_stack_drain_bench.c $(REMOTE_STACK_SRC)
BENCH_MULTICLASS_REMOTE_SRC := $(ROOT)/bench/hz10_multiclass_remote_bench.c $(ROOT)/src/hz10_size_class.c $(REMOTE_STACK_SRC)
BENCH_REMOTE_RMW_MICRO_SRC := $(ROOT)/bench/hz10_remote_rmw_microbench.c $(ROOT)/src/hz10_platform.c

POOLED_PAGE_SRC := $(ROOT)/src/hz10_pooled_page.c $(ROOT)/src/hz10_page_pool.c $(REMOTE_STACK_SRC)
SMOKE_POOLED_PAGE_SRC := $(ROOT)/tests/hz10_bounded_page_pool_smoke.c $(POOLED_PAGE_SRC)
BENCH_POOLED_PAGE_SRC := $(ROOT)/bench/hz10_bounded_page_pool_bench.c $(POOLED_PAGE_SRC)

CLASS_PAGES_SRC := $(ROOT)/src/hz10_class_pages.c $(ROOT)/src/hz10_retired_ready.c $(POOLED_PAGE_SRC)
SMOKE_CLASS_PAGES_SRC := $(ROOT)/tests/hz10_class_pages_smoke.c $(CLASS_PAGES_SRC)

PUBLIC_ENTRY_SRC := $(ROOT)/src/hz10_public_entry.c $(ROOT)/src/hz10_public_entry_owner.c $(ROOT)/src/hz10_class_pages.c $(ROOT)/src/hz10_retired_ready.c $(ROOT)/src/hz10_size_class.c $(ROOT)/src/hz10_large_alloc.c $(POOLED_PAGE_SRC)
SMOKE_PUBLIC_ENTRY_SRC := $(ROOT)/tests/hz10_public_entry_smoke.c $(PUBLIC_ENTRY_SRC)
SMOKE_PUBLIC_ENTRY_OWNER_SRC := $(ROOT)/tests/hz10_public_entry_owner_smoke.c $(PUBLIC_ENTRY_SRC)
BENCH_PUBLIC_ENTRY_SRC := $(ROOT)/bench/hz10_public_entry_bench.c $(PUBLIC_ENTRY_SRC)
BENCH_REMOTE_BATCH_LOCALITY_SRC := $(ROOT)/bench/hz10_public_entry_remote_batch_locality_bench.c $(PUBLIC_ENTRY_SRC)
STAGE_COST_SRC := $(ROOT)/bench/hz10_public_entry_stage_cost_bench.c $(ROOT)/src/hz10_retired_ready.c $(POOLED_PAGE_SRC)
LOCAL_PATH_SRC := $(ROOT)/bench/hz10_public_entry_local_path_bench.c $(PUBLIC_ENTRY_SRC)
SHIM_SRC := $(ROOT)/src/hz10_shim.c $(ROOT)/src/hz10_shim_orphan_probes.c $(PUBLIC_ENTRY_SRC)
SMOKE_SHIM_API_SRC := $(ROOT)/tests/hz10_shim_api_smoke.c

all: smoke-pagemap-route smoke-pagemap-route-diff bench-pagemap-route \
	smoke-freelist-page bench-freelist-page \
	smoke-remote-stack-drain bench-remote-stack-drain bench-multiclass-remote \
	smoke-bounded-page-pool bench-bounded-page-pool smoke-class-pages \
	smoke-size-class \
	smoke-public-entry bench-public-entry bench-class-pages-scan \
	smoke-retired-ready bench-retired-ready preload smoke-shim-api

$(ROOT)/hz10_pagemap_route_smoke: $(SMOKE_PAGEMAP_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_PAGEMAP_SRC) $(LDFLAGS) $(LDLIBS)

smoke-pagemap-route: $(ROOT)/hz10_pagemap_route_smoke
	$(ROOT)/hz10_pagemap_route_smoke

$(ROOT)/hz10_pagemap_route_diff_smoke: $(SMOKE_PAGEMAP_DIFF_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_PAGEMAP_DIFF_SRC) $(LDFLAGS) $(LDLIBS)

smoke-pagemap-route-diff: $(ROOT)/hz10_pagemap_route_diff_smoke
	$(ROOT)/hz10_pagemap_route_diff_smoke

$(ROOT)/hz10_pagemap_route_bench: $(BENCH_PAGEMAP_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_PAGEMAP_SRC) $(LDFLAGS) $(LDLIBS)

bench-pagemap-route: $(ROOT)/hz10_pagemap_route_bench
	$(ROOT)/hz10_pagemap_route_bench

$(ROOT)/hz10_freelist_page_smoke: $(SMOKE_FREELIST_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_FREELIST_SRC) $(LDFLAGS) $(LDLIBS)

smoke-freelist-page: $(ROOT)/hz10_freelist_page_smoke
	$(ROOT)/hz10_freelist_page_smoke

$(ROOT)/hz10_freelist_page_bench: $(BENCH_FREELIST_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_FREELIST_SRC) $(LDFLAGS) $(LDLIBS)

bench-freelist-page: $(ROOT)/hz10_freelist_page_bench
	$(ROOT)/hz10_freelist_page_bench

$(ROOT)/hz10_remote_stack_drain_smoke: $(SMOKE_REMOTE_STACK_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_REMOTE_STACK_SRC) $(LDFLAGS) $(LDLIBS)

smoke-remote-stack-drain: $(ROOT)/hz10_remote_stack_drain_smoke
	$(ROOT)/hz10_remote_stack_drain_smoke

$(ROOT)/hz10_remote_stack_drain_bench: $(BENCH_REMOTE_STACK_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_REMOTE_STACK_SRC) $(LDFLAGS) $(LDLIBS)

bench-remote-stack-drain: $(ROOT)/hz10_remote_stack_drain_bench
	$(ROOT)/hz10_remote_stack_drain_bench

$(ROOT)/hz10_multiclass_remote_bench: $(BENCH_MULTICLASS_REMOTE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_MULTICLASS_REMOTE_SRC) $(LDFLAGS) $(LDLIBS)

bench-multiclass-remote: $(ROOT)/hz10_multiclass_remote_bench
	$(ROOT)/hz10_multiclass_remote_bench

$(ROOT)/hz10_remote_rmw_microbench: $(BENCH_REMOTE_RMW_MICRO_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_REMOTE_RMW_MICRO_SRC) $(LDFLAGS) $(LDLIBS)

bench-remote-rmw-micro: $(ROOT)/hz10_remote_rmw_microbench
	$(ROOT)/hz10_remote_rmw_microbench

$(ROOT)/hz10_public_entry_stage_cost_bench: $(STAGE_COST_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(STAGE_COST_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-stage-cost: $(ROOT)/hz10_public_entry_stage_cost_bench
	$(ROOT)/hz10_public_entry_stage_cost_bench

$(ROOT)/hz10_public_entry_local_path_bench: $(LOCAL_PATH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(LOCAL_PATH_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-local-path: $(ROOT)/hz10_public_entry_local_path_bench
	$(ROOT)/hz10_public_entry_local_path_bench

$(ROOT)/hz10_bounded_page_pool_smoke: $(SMOKE_POOLED_PAGE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_POOLED_PAGE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-bounded-page-pool: $(ROOT)/hz10_bounded_page_pool_smoke
	$(ROOT)/hz10_bounded_page_pool_smoke

$(ROOT)/hz10_bounded_page_pool_bench: $(BENCH_POOLED_PAGE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_POOLED_PAGE_SRC) $(LDFLAGS) $(LDLIBS)

bench-bounded-page-pool: $(ROOT)/hz10_bounded_page_pool_bench
	$(ROOT)/hz10_bounded_page_pool_bench

$(ROOT)/hz10_class_pages_smoke: $(SMOKE_CLASS_PAGES_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_CLASS_PAGES_SRC) $(LDFLAGS) $(LDLIBS)

smoke-class-pages: $(ROOT)/hz10_class_pages_smoke
	$(ROOT)/hz10_class_pages_smoke

SIZE_CLASS_SRC := $(ROOT)/src/hz10_size_class.c $(PAGEMAP_SRC)
SMOKE_SIZE_CLASS_SRC := $(ROOT)/tests/hz10_size_class_smoke.c $(SIZE_CLASS_SRC)

$(ROOT)/hz10_size_class_smoke: $(SMOKE_SIZE_CLASS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_SIZE_CLASS_SRC) $(LDFLAGS) $(LDLIBS)

smoke-size-class: $(ROOT)/hz10_size_class_smoke
	$(ROOT)/hz10_size_class_smoke

$(ROOT)/hz10_public_entry_smoke: $(SMOKE_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/hz10_public_entry_owner_smoke: $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-owner: $(ROOT)/hz10_public_entry_owner_smoke
	$(ROOT)/hz10_public_entry_owner_smoke

smoke-public-entry: $(ROOT)/hz10_public_entry_smoke $(ROOT)/hz10_public_entry_owner_smoke
	$(ROOT)/hz10_public_entry_smoke
	$(ROOT)/hz10_public_entry_owner_smoke

$(ROOT)/hz10_public_entry_orphan_adoption_smoke: $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-orphan-adoption: $(ROOT)/hz10_public_entry_orphan_adoption_smoke
	$(ROOT)/hz10_public_entry_orphan_adoption_smoke

$(ROOT)/hz10_public_entry_orphan_partial_smoke: $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-orphan-partial: $(ROOT)/hz10_public_entry_orphan_partial_smoke
	$(ROOT)/hz10_public_entry_orphan_partial_smoke

$(ROOT)/hz10_public_entry_orphan_partial_front_smoke: $(SMOKE_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 -DHZ10_ENABLE_FRONT_CACHE=1 -DHZ10_ENABLE_FRONT_CACHE_STATS=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-orphan-partial-front: $(ROOT)/hz10_public_entry_orphan_partial_front_smoke
	$(ROOT)/hz10_public_entry_orphan_partial_front_smoke

$(ROOT)/hz10_public_entry_owner_orphan_partial_front_smoke: $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 -DHZ10_ENABLE_FRONT_CACHE=1 -DHZ10_ENABLE_FRONT_CACHE_STATS=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_OWNER_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-owner-orphan-partial-front: $(ROOT)/hz10_public_entry_owner_orphan_partial_front_smoke
	$(ROOT)/hz10_public_entry_owner_orphan_partial_front_smoke

$(ROOT)/hz10_public_entry_retired_local_smoke: $(SMOKE_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) -DHZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-retired-local: $(ROOT)/hz10_public_entry_retired_local_smoke
	$(ROOT)/hz10_public_entry_retired_local_smoke

$(ROOT)/hz10_public_entry_retired_local_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1 $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-retired-local: $(ROOT)/hz10_public_entry_retired_local_bench
	$(ROOT)/hz10_public_entry_retired_local_bench

$(ROOT)/hz10_public_entry_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry: $(ROOT)/hz10_public_entry_bench
	$(ROOT)/hz10_public_entry_bench

# Current shim default: orphan + partial adoption plus the fine size-class
# table. The coarse partial-adoption sibling remains available as
# libhz10_orphan_partial.so for immediate A/B rollback.
$(ROOT)/libhz10.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 $(INC) -shared -Wl,-soname,libhz10.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload: $(ROOT)/libhz10.so

# HZ10BumpInit-L0: default lane + bump-init (lazy page init) sibling for
# measurement of the cold-start page-fault win on local-only rows.
$(ROOT)/libhz10_bump.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 -DHZ10_ENABLE_BUMP_INIT=1 $(INC) -shared -Wl,-soname,libhz10_bump.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-bump: $(ROOT)/libhz10_bump.so

# Stable sibling preload artifacts for side-by-side macro matrix rows.
$(ROOT)/libhz10_base.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) $(INC) -shared -Wl,-soname,libhz10_base.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-base: $(ROOT)/libhz10_base.so

# Compatibility sibling for older scripts that named the fine candidate
# explicitly. It intentionally tracks the current default.
$(ROOT)/libhz10_fine.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 $(INC) -shared -Wl,-soname,libhz10_fine.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-fine: $(ROOT)/libhz10_fine.so

$(ROOT)/libhz10_thread_stats.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 -DHZ10_ENABLE_SHIM_THREAD_EXIT_STATS=1 $(INC) -shared -Wl,-soname,libhz10_thread_stats.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-thread-stats: $(ROOT)/libhz10_thread_stats.so

$(ROOT)/libhz10_front.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 -DHZ10_ENABLE_FRONT_CACHE=1 $(INC) -shared -Wl,-soname,libhz10_front.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-front: $(ROOT)/libhz10_front.so

$(ROOT)/libhz10_orphan.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 $(INC) -shared -Wl,-soname,libhz10_orphan.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-orphan-adoption: $(ROOT)/libhz10_orphan.so

# Coarse rollback for the current shim default: orphan + partial adoption,
# but without fine size classes.
$(ROOT)/libhz10_coarse.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 $(INC) -shared -Wl,-soname,libhz10_coarse.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-coarse: $(ROOT)/libhz10_coarse.so

# Compatibility name for the old partial-adoption candidate.
$(ROOT)/libhz10_orphan_partial.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_ORPHAN_ACTIVE_ADOPTION=1 -DHZ10_ENABLE_PARTIAL_ORPHAN_ADOPTION=1 $(INC) -shared -Wl,-soname,libhz10_orphan_partial.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-orphan-partial: $(ROOT)/libhz10_orphan_partial.so

# Manual probe lanes below intentionally clobber libhz10.so.
preload-fine-size-classes: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 $(INC) -shared -Wl,-soname,libhz10.so -o $(ROOT)/libhz10.so $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-retired-local: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1 $(INC) -shared -Wl,-soname,libhz10.so -o $(ROOT)/libhz10.so $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload-fine-retired-local: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ10_ENABLE_FINE_SIZE_CLASSES=1 -DHZ10_ENABLE_RETIRED_LOCAL_IDLE_RECLAIM=1 $(INC) -shared -Wl,-soname,libhz10.so -o $(ROOT)/libhz10.so $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

$(ROOT)/hz10_shim_api_smoke: $(SMOKE_SHIM_API_SRC) $(ROOT)/libhz10.so $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_SHIM_API_SRC) $(LDFLAGS) -ldl $(LDLIBS)

smoke-shim-api: $(ROOT)/hz10_shim_api_smoke $(ROOT)/libhz10.so
	cd $(ROOT) && $(ROOT)/hz10_shim_api_smoke

smoke-shim-foreign: $(ROOT)/libhz10.so
	$(ROOT)/scripts/run_hz10_shim_smoke.sh

bench-macro-preload: $(ROOT)/libhz10.so $(ROOT)/libhz10_base.so $(ROOT)/libhz10_fine.so $(ROOT)/libhz10_thread_stats.so $(ROOT)/libhz10_front.so $(ROOT)/libhz10_coarse.so $(ROOT)/libhz10_orphan.so $(ROOT)/libhz10_orphan_partial.so
	$(ROOT)/scripts/run_hz10_macro_preload_matrix.sh

bench-macro-matrix: bench-macro-preload

bench-larson-thread-churn-attribution: $(ROOT)/libhz10.so $(ROOT)/libhz10_base.so $(ROOT)/libhz10_fine.so $(ROOT)/libhz10_thread_stats.so $(ROOT)/libhz10_front.so $(ROOT)/libhz10_coarse.so $(ROOT)/libhz10_orphan.so $(ROOT)/libhz10_orphan_partial.so
	$(ROOT)/scripts/run_hz10_larson_thread_churn_attribution.sh

# HZ10FrontCache-L1 opt-in lanes (docs/HZ10_FRONT_CACHE_DESIGN_L0.md).
# The smoke lane adds the event counters; the bench lanes build with the
# flag only, so counter stores never sit inside a timed window.
FRONT_CACHE_FLAG := -DHZ10_ENABLE_FRONT_CACHE=1
FRONT_CACHE_ARRAY_FLAG := $(FRONT_CACHE_FLAG) -DHZ10_ENABLE_FRONT_CACHE_ARRAY=1

$(ROOT)/hz10_public_entry_front_smoke: $(SMOKE_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(FRONT_CACHE_FLAG) -DHZ10_ENABLE_FRONT_CACHE_STATS=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-front: $(ROOT)/hz10_public_entry_front_smoke
	$(ROOT)/hz10_public_entry_front_smoke

$(ROOT)/hz10_public_entry_front_array_smoke: $(SMOKE_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(FRONT_CACHE_ARRAY_FLAG) -DHZ10_ENABLE_FRONT_CACHE_STATS=1 $(INC) -o $@ $(SMOKE_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

smoke-public-entry-front-array: $(ROOT)/hz10_public_entry_front_array_smoke
	$(ROOT)/hz10_public_entry_front_array_smoke

smoke-tsan-aslr-off:
	command -v setarch >/dev/null
	$(MAKE) -B DEBUG_CFLAGS="$(TSAN_DEBUG_CFLAGS)" LDFLAGS="$(TSAN_LDFLAGS)" \
		"$(ROOT)/hz10_pagemap_route_smoke" \
		"$(ROOT)/hz10_pagemap_route_diff_smoke" \
		"$(ROOT)/hz10_public_entry_smoke" \
		"$(ROOT)/hz10_public_entry_owner_smoke" \
		"$(ROOT)/hz10_public_entry_orphan_adoption_smoke" \
		"$(ROOT)/hz10_public_entry_orphan_partial_smoke" \
		"$(ROOT)/hz10_public_entry_orphan_partial_front_smoke" \
		"$(ROOT)/hz10_public_entry_owner_orphan_partial_front_smoke" \
		"$(ROOT)/hz10_public_entry_front_smoke"
	arch=$$(uname -m); \
	for bin in hz10_pagemap_route_smoke hz10_pagemap_route_diff_smoke \
		hz10_public_entry_smoke hz10_public_entry_owner_smoke \
		hz10_public_entry_orphan_adoption_smoke \
		hz10_public_entry_orphan_partial_smoke \
		hz10_public_entry_orphan_partial_front_smoke \
		hz10_public_entry_owner_orphan_partial_front_smoke \
		hz10_public_entry_front_smoke; do \
		echo "== $$bin (TSan, ASLR off) =="; \
		setarch "$$arch" -R "$(ROOT)/$$bin" || exit $$?; \
	done

$(ROOT)/hz10_public_entry_front_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(FRONT_CACHE_FLAG) $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-front: $(ROOT)/hz10_public_entry_front_bench
	$(ROOT)/hz10_public_entry_front_bench

$(ROOT)/hz10_public_entry_front_array_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(FRONT_CACHE_ARRAY_FLAG) $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-front-array: $(ROOT)/hz10_public_entry_front_array_bench
	$(ROOT)/hz10_public_entry_front_array_bench

$(ROOT)/hz10_public_entry_local_path_front_bench: $(LOCAL_PATH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(FRONT_CACHE_FLAG) $(INC) -o $@ $(LOCAL_PATH_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-local-path-front: $(ROOT)/hz10_public_entry_local_path_front_bench
	$(ROOT)/hz10_public_entry_local_path_front_bench

$(ROOT)/hz10_public_entry_local_path_front_array_bench: $(LOCAL_PATH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(FRONT_CACHE_ARRAY_FLAG) $(INC) -o $@ $(LOCAL_PATH_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-local-path-front-array: $(ROOT)/hz10_public_entry_local_path_front_array_bench
	$(ROOT)/hz10_public_entry_local_path_front_array_bench

$(ROOT)/hz10_public_entry_active_scan_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ10_ENABLE_ACTIVE_SCAN_STATS=1 $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-active-scan: $(ROOT)/hz10_public_entry_active_scan_bench
	$(ROOT)/hz10_public_entry_active_scan_bench

$(ROOT)/hz10_public_entry_active_mtf_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ10_ENABLE_ACTIVE_SCAN_STATS=1 -DHZ10_ENABLE_ACTIVE_MTF=1 $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-active-mtf: $(ROOT)/hz10_public_entry_active_mtf_bench
	$(ROOT)/hz10_public_entry_active_mtf_bench

$(ROOT)/hz10_public_entry_two_slot_bench: $(BENCH_PUBLIC_ENTRY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ10_ENABLE_ACTIVE_SCAN_STATS=1 -DHZ10_ENABLE_TWO_SLOT_STATS=1 $(INC) -o $@ $(BENCH_PUBLIC_ENTRY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-two-slot: $(ROOT)/hz10_public_entry_two_slot_bench
	$(ROOT)/hz10_public_entry_two_slot_bench

$(ROOT)/hz10_public_entry_remote_batch_locality_bench: $(BENCH_REMOTE_BATCH_LOCALITY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_REMOTE_BATCH_LOCALITY_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-remote-batch-locality: $(ROOT)/hz10_public_entry_remote_batch_locality_bench
	$(ROOT)/hz10_public_entry_remote_batch_locality_bench

CLASS_PAGES_SCAN_BENCH_SRC := $(ROOT)/bench/hz10_class_pages_scan_bench.c $(PUBLIC_ENTRY_SRC)

$(ROOT)/hz10_class_pages_scan_bench: $(CLASS_PAGES_SCAN_BENCH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(CLASS_PAGES_SCAN_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

RETIRED_READY_SRC := $(ROOT)/src/hz10_retired_ready.c $(REMOTE_STACK_SRC)
SMOKE_RETIRED_READY_SRC := $(ROOT)/tests/hz10_retired_ready_smoke.c $(RETIRED_READY_SRC)
BENCH_RETIRED_READY_SRC := $(ROOT)/bench/hz10_retired_ready_bench.c $(RETIRED_READY_SRC)

$(ROOT)/hz10_retired_ready_smoke: $(SMOKE_RETIRED_READY_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_RETIRED_READY_SRC) $(LDFLAGS) $(LDLIBS)

smoke-retired-ready: $(ROOT)/hz10_retired_ready_smoke
	$(ROOT)/hz10_retired_ready_smoke

$(ROOT)/hz10_retired_ready_bench: $(BENCH_RETIRED_READY_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_RETIRED_READY_SRC) $(LDFLAGS) $(LDLIBS)

bench-retired-ready: $(ROOT)/hz10_retired_ready_bench
	$(ROOT)/hz10_retired_ready_bench

STEADY_STATE_BENCH_SRC := $(ROOT)/bench/hz10_public_entry_steady_state_bench.c $(PUBLIC_ENTRY_SRC)

$(ROOT)/hz10_public_entry_steady_state_bench: $(STEADY_STATE_BENCH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(STEADY_STATE_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-steady-state: $(ROOT)/hz10_public_entry_steady_state_bench
	$(ROOT)/hz10_public_entry_steady_state_bench

THREAD_REUSE_BENCH_SRC := $(ROOT)/bench/hz10_public_entry_thread_reuse_bench.c $(PUBLIC_ENTRY_SRC)

$(ROOT)/hz10_public_entry_thread_reuse_bench: $(THREAD_REUSE_BENCH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(THREAD_REUSE_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-public-entry-thread-reuse: $(ROOT)/hz10_public_entry_thread_reuse_bench
	$(ROOT)/hz10_public_entry_thread_reuse_bench

bench-class-pages-scan: $(ROOT)/hz10_class_pages_scan_bench
	$(ROOT)/hz10_class_pages_scan_bench

hz10-standalone-check:
	$(ROOT)/scripts/check_hz10_standalone.sh

hz10-rss-guard:
	$(ROOT)/scripts/run_hz10_rss_guard.sh

bench-hz8-public-purge-matrix:
	$(ROOT)/scripts/run_hz10_hz8_public_purge_matrix.sh

clean:
	rm -f $(ROOT)/hz10_pagemap_route_smoke $(ROOT)/hz10_pagemap_route_bench \
		$(ROOT)/hz10_pagemap_route_diff_smoke \
		$(ROOT)/hz10_freelist_page_smoke $(ROOT)/hz10_freelist_page_bench \
		$(ROOT)/hz10_remote_stack_drain_smoke $(ROOT)/hz10_remote_stack_drain_bench \
		$(ROOT)/hz10_multiclass_remote_bench $(ROOT)/hz10_remote_rmw_microbench \
		$(ROOT)/hz10_bounded_page_pool_smoke $(ROOT)/hz10_bounded_page_pool_bench \
		$(ROOT)/hz10_class_pages_smoke \
		$(ROOT)/hz10_public_entry_smoke $(ROOT)/hz10_public_entry_owner_smoke \
		$(ROOT)/hz10_public_entry_orphan_adoption_smoke \
		$(ROOT)/hz10_public_entry_orphan_partial_smoke \
		$(ROOT)/hz10_public_entry_retired_local_smoke \
		$(ROOT)/hz10_public_entry_bench $(ROOT)/hz10_public_entry_retired_local_bench \
		$(ROOT)/hz10_public_entry_active_scan_bench \
		$(ROOT)/hz10_public_entry_active_mtf_bench \
		$(ROOT)/hz10_public_entry_remote_batch_locality_bench \
		$(ROOT)/hz10_public_entry_stage_cost_bench \
		$(ROOT)/hz10_public_entry_local_path_bench \
		$(ROOT)/hz10_public_entry_front_smoke \
		$(ROOT)/hz10_public_entry_front_array_smoke \
		$(ROOT)/hz10_public_entry_front_bench \
		$(ROOT)/hz10_public_entry_front_array_bench \
		$(ROOT)/hz10_public_entry_local_path_front_bench \
		$(ROOT)/hz10_public_entry_local_path_front_array_bench \
		$(ROOT)/hz10_public_entry_two_slot_bench \
		$(ROOT)/hz10_class_pages_scan_bench $(ROOT)/hz10_size_class_smoke \
		$(ROOT)/hz10_retired_ready_smoke $(ROOT)/hz10_retired_ready_bench \
		$(ROOT)/hz10_public_entry_steady_state_bench \
		$(ROOT)/hz10_public_entry_thread_reuse_bench \
		$(ROOT)/libhz10.so $(ROOT)/libhz10_base.so \
		$(ROOT)/libhz10_fine.so $(ROOT)/libhz10_orphan.so \
		$(ROOT)/libhz10_orphan_partial.so \
		$(ROOT)/hz10_shim_api_smoke
