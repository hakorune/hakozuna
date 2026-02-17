# hz4 Makefile
# Box Theory: 最小構成でテスト可能に

CC ?= gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c11
CFLAGS_DEBUG = -O0 -g -Wall -Wextra -Werror -std=c11 -DHZ4_FAILFAST=1
TLS_CFLAGS =
HZ4_TLS_INITIAL_EXEC ?= 1

ifeq ($(HZ4_TLS_INITIAL_EXEC),1)
TLS_CFLAGS += -ftls-model=initial-exec
endif

# Extra compile-time defs for SSOT A/B (e.g., -DHZ4_PAGE_META_SEPARATE=1 ...)
HZ4_DEFS_EXTRA ?=
CFLAGS += $(HZ4_DEFS_EXTRA)
CFLAGS_DEBUG += $(HZ4_DEFS_EXTRA)

CORE_DIR = core
INC_DIR = include
SRC_DIR = src
TEST_DIR = tests
OUT_DIR = out

LIB_TARGET = libhakozuna_hz4.so

CORE_HEADERS = \
	$(CORE_DIR)/hz4_config.h \
	$(CORE_DIR)/hz4_types.h \
	$(CORE_DIR)/hz4_page.h \
	$(CORE_DIR)/hz4_seg.h \
	$(CORE_DIR)/hz4_tls.h \
	$(CORE_DIR)/hz4_pagetag.h \
	$(CORE_DIR)/hz4_xfer.h \
	$(CORE_DIR)/hz4_tcbox.h

CORE_INCS = \
	$(CORE_DIR)/hz4_segq.inc \
	$(CORE_DIR)/hz4_remote_flush.inc \
	$(CORE_DIR)/hz4_collect.inc \
	$(CORE_DIR)/hz4_inbox.inc \
	$(CORE_DIR)/hz4_xfer.inc \
	$(CORE_DIR)/hz4_tcbox.inc

INC_HEADERS = \
	$(INC_DIR)/hz4_sizeclass.h \
	$(INC_DIR)/hz4_os.h \
	$(INC_DIR)/hz4_segment.h \
	$(INC_DIR)/hz4_tls_init.h \
	$(INC_DIR)/hz4_tcache.h \
	$(INC_DIR)/hz4_mid.h \
	$(INC_DIR)/hz4_large.h

MID_INCS = \
	$(SRC_DIR)/hz4_mid_stats.inc \
	$(SRC_DIR)/hz4_mid_tls_cache.inc \
	$(SRC_DIR)/hz4_mid_page_supply.inc \
	$(SRC_DIR)/hz4_mid_malloc.inc \
	$(SRC_DIR)/hz4_mid_free.inc \
	$(SRC_DIR)/hz4_mid_freelist_boundary.inc \
	$(SRC_DIR)/hz4_mid_owner_remote.inc \
	$(SRC_DIR)/hz4_mid_batch_cache.inc

.PHONY: all all_stable all_perf all_perf_lib all_rss all_rss_lib all_rss_larson all_rss_larson_lib all_rss_larson_stable_lib clean test test_debug bench

# ベンチ用（統計 ON、最適化 ON）
CFLAGS_BENCH = -O2 -Wall -Wextra -Werror -std=c11 -DHZ4_STATS=1
CFLAGS_BENCH += $(HZ4_DEFS_EXTRA)

HZ4_USE_LTO ?= 0
ifeq ($(HZ4_USE_LTO),1)
CFLAGS += -flto
CFLAGS_BENCH += -flto
endif

# ============================================================================
# Lane presets (SSOT: avoid mixing flags)
# ============================================================================
HZ4_DEFS_PERF_LANE ?= -DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0
HZ4_DEFS_RSS_LANE ?= -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_DECOMMIT_DELAY_QUEUE=1 -DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1
HZ4_DEFS_RSS_LARSON_LANE ?= \
	-DHZ4_OS_STATS=1 \
	-DHZ4_PAGE_META_SEPARATE=1 \
	-DHZ4_PAGE_DECOMMIT=1 \
	-DHZ4_DECOMMIT_DELAY_QUEUE=1 \
	-DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1 \
	-DHZ4_SEG_RELEASE_EMPTY=0 \
	-DHZ4_RSSRETURN_ADAPTIVE_BUDGET=1 \
	-DHZ4_DECOMMIT_DELAY_TICKS=0 \
	-DHZ4_RSSRETURN_THRESH_PAGES=1 \
	-DHZ4_RSSRETURN_BATCH_PAGES=64 \
	-DHZ4_RSSRETURN_BATCH_MAX_PAGES=128 \
	-DHZ4_RSSRETURN_BATCH_DIV=2 \
	-DHZ4_RSSRETURN_SAFETY_PERIOD=1

HZ4_DEFS_RSS_LARSON_STABLE_LANE ?= \
	-DHZ4_PAGE_META_SEPARATE=1 \
	-DHZ4_PAGE_DECOMMIT=1 \
	-DHZ4_DECOMMIT_DELAY_QUEUE=1 \
	-DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1 \
	-DHZ4_SEG_RELEASE_EMPTY=1 \
	-DHZ4_RSSRETURN_ADAPTIVE_BUDGET=1 \
	-DHZ4_DECOMMIT_DELAY_TICKS=16 \
	-DHZ4_RSSRETURN_THRESH_PAGES=32 \
	-DHZ4_RSSRETURN_BATCH_PAGES=64 \
	-DHZ4_RSSRETURN_BATCH_MAX_PAGES=128 \
	-DHZ4_RSSRETURN_BATCH_DIV=2 \
	-DHZ4_RSSRETURN_SAFETY_PERIOD=1

TEST_TARGETS = \
	$(OUT_DIR)/hz4_segq_test \
	$(OUT_DIR)/hz4_basic_test \
	$(OUT_DIR)/hz4_cph_disjoint_test \
	$(OUT_DIR)/hz4_route_smoke \
	$(OUT_DIR)/hz4_route_edges \
	$(OUT_DIR)/hz4_mid_cross_free_hang_guard

all: $(LIB_TARGET) $(TEST_TARGETS)

all_stable:
	$(MAKE) clean all

# perf lane is library-only to avoid test macro collisions.
all_perf:
	$(MAKE) clean all_perf_lib

all_perf_lib:
	$(MAKE) $(LIB_TARGET) HZ4_DEFS_EXTRA='$(HZ4_DEFS_PERF_LANE)'

all_rss:
	@echo "[DEPRECATED] all_rss is kept for compatibility; prefer all_stable or all_rss_larson_stable_lib." >&2
	$(MAKE) clean all HZ4_DEFS_EXTRA='$(HZ4_DEFS_RSS_LANE)'

all_rss_lib:
	@echo "[DEPRECATED] all_rss_lib is kept for compatibility; prefer all_stable or all_rss_larson_stable_lib." >&2
	$(MAKE) $(LIB_TARGET) HZ4_DEFS_EXTRA='$(HZ4_DEFS_RSS_LANE)'

all_rss_larson:
	@echo "[DEPRECATED] all_rss_larson is unstable; prefer all_rss_larson_stable_lib." >&2
	$(MAKE) clean all HZ4_DEFS_EXTRA='$(HZ4_DEFS_RSS_LARSON_LANE)'

all_rss_larson_lib:
	@echo "[DEPRECATED] all_rss_larson_lib is unstable; prefer all_rss_larson_stable_lib." >&2
	$(MAKE) $(LIB_TARGET) HZ4_DEFS_EXTRA='$(HZ4_DEFS_RSS_LARSON_LANE)'

all_rss_larson_stable_lib:
	$(MAKE) $(LIB_TARGET) HZ4_DEFS_EXTRA='$(HZ4_DEFS_RSS_LARSON_STABLE_LANE)'

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# ============================================================================
# Unit Tests
# ============================================================================
$(OUT_DIR)/hz4_segq_test: $(TEST_DIR)/hz4_segq_test.c $(CORE_HEADERS) $(CORE_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(CORE_DIR) -I$(INC_DIR) -o $@ $< -lpthread

$(OUT_DIR)/hz4_basic_test: $(TEST_DIR)/hz4_basic_test.c $(CORE_HEADERS) $(CORE_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(CORE_DIR) -I$(INC_DIR) -o $@ $< -lpthread

$(OUT_DIR)/hz4_cph_disjoint_test: $(TEST_DIR)/hz4_cph_disjoint_test.c $(CORE_HEADERS) $(CORE_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_DEBUG) -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_CENTRAL_PAGEHEAP=1 -DHZ4_CPH_2TIER=1 -I$(CORE_DIR) -I$(INC_DIR) -o $@ $< \
		$(SRC_DIR)/hz4_os.c $(SRC_DIR)/hz4_os_research.c $(SRC_DIR)/hz4_central_pageheap.c -lpthread

$(OUT_DIR)/hz4_route_smoke: $(TEST_DIR)/hz4_route_smoke.c $(LIB_SRCS) $(CORE_HEADERS) $(CORE_INCS) $(INC_HEADERS) $(MID_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(CORE_DIR) -I$(INC_DIR) -o $@ $(TEST_DIR)/hz4_route_smoke.c \
		$(SRC_DIR)/hz4_os.c $(SRC_DIR)/hz4_os_research.c $(SRC_DIR)/hz4_segment.c $(SRC_DIR)/hz4_tls_init.c \
		$(SRC_DIR)/hz4_tcache.c $(SRC_DIR)/hz4_mid.c $(SRC_DIR)/hz4_large.c $(SRC_DIR)/hz4_inbox.c $(SRC_DIR)/hz4_pagetag.c \
		$(SRC_DIR)/hz4_central_pageheap.c $(SRC_DIR)/hz4_tcbox.c -lpthread

$(OUT_DIR)/hz4_route_edges: $(TEST_DIR)/hz4_route_edges.c $(LIB_SRCS) $(CORE_HEADERS) $(CORE_INCS) $(INC_HEADERS) $(MID_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(CORE_DIR) -I$(INC_DIR) -o $@ $(TEST_DIR)/hz4_route_edges.c \
		$(SRC_DIR)/hz4_os.c $(SRC_DIR)/hz4_os_research.c $(SRC_DIR)/hz4_segment.c $(SRC_DIR)/hz4_tls_init.c \
		$(SRC_DIR)/hz4_tcache.c $(SRC_DIR)/hz4_mid.c $(SRC_DIR)/hz4_large.c $(SRC_DIR)/hz4_shim.c $(SRC_DIR)/hz4_inbox.c $(SRC_DIR)/hz4_pagetag.c \
		$(SRC_DIR)/hz4_central_pageheap.c $(SRC_DIR)/hz4_tcbox.c -lpthread

$(OUT_DIR)/hz4_mid_cross_free_hang_guard: $(TEST_DIR)/hz4_mid_cross_free_hang_guard.c $(LIB_SRCS) $(CORE_HEADERS) $(CORE_INCS) $(INC_HEADERS) $(MID_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(CORE_DIR) -I$(INC_DIR) -o $@ $(TEST_DIR)/hz4_mid_cross_free_hang_guard.c \
		$(SRC_DIR)/hz4_os.c $(SRC_DIR)/hz4_os_research.c $(SRC_DIR)/hz4_segment.c $(SRC_DIR)/hz4_tls_init.c \
		$(SRC_DIR)/hz4_tcache.c $(SRC_DIR)/hz4_mid.c $(SRC_DIR)/hz4_large.c $(SRC_DIR)/hz4_inbox.c $(SRC_DIR)/hz4_pagetag.c \
		$(SRC_DIR)/hz4_central_pageheap.c $(SRC_DIR)/hz4_xfer.c $(SRC_DIR)/hz4_tcbox.c -lpthread

# ============================================================================
# Run Tests
# ============================================================================
test: $(TEST_TARGETS)
	@echo "=== Running hz4_segq_test ==="
	./$(OUT_DIR)/hz4_segq_test
	@echo ""
	@echo "=== Running hz4_basic_test ==="
	./$(OUT_DIR)/hz4_basic_test
	@echo ""
	@echo "=== Running hz4_cph_disjoint_test ==="
	./$(OUT_DIR)/hz4_cph_disjoint_test
	@echo ""
	@echo "=== Running hz4_route_smoke ==="
	./$(OUT_DIR)/hz4_route_smoke
	@echo ""
	@echo "=== Running hz4_route_edges ==="
	./$(OUT_DIR)/hz4_route_edges
	@echo ""
	@echo "=== Running hz4_mid_cross_free_hang_guard ==="
	./$(OUT_DIR)/hz4_mid_cross_free_hang_guard
	@echo ""
	@echo "All tests passed!"

test_debug: $(TEST_TARGETS)
	@echo "=== Running hz4_segq_test (debug) ==="
	./$(OUT_DIR)/hz4_segq_test
	@echo ""
	@echo "=== Running hz4_basic_test (debug) ==="
	./$(OUT_DIR)/hz4_basic_test
	@echo ""
	@echo "=== Running hz4_cph_disjoint_test (debug) ==="
	./$(OUT_DIR)/hz4_cph_disjoint_test
	@echo ""
	@echo "=== Running hz4_route_smoke (debug) ==="
	./$(OUT_DIR)/hz4_route_smoke
	@echo ""
	@echo "=== Running hz4_route_edges (debug) ==="
	./$(OUT_DIR)/hz4_route_edges
	@echo ""
	@echo "=== Running hz4_mid_cross_free_hang_guard (debug) ==="
	./$(OUT_DIR)/hz4_mid_cross_free_hang_guard

# ============================================================================
# Benchmark
# ============================================================================
$(OUT_DIR)/bench_hz4: $(TEST_DIR)/hz4_bench.c $(CORE_HEADERS) $(CORE_INCS) | $(OUT_DIR)
	$(CC) $(CFLAGS_BENCH) -I$(CORE_DIR) -I$(INC_DIR) -o $@ $< -lpthread

# ============================================================================
# Shared Library (LD_PRELOAD)
# ============================================================================
LIB_SRCS = \
	$(SRC_DIR)/hz4_os.c \
	$(SRC_DIR)/hz4_os_research.c \
	$(SRC_DIR)/hz4_segment.c \
	$(SRC_DIR)/hz4_tls_init.c \
	$(SRC_DIR)/hz4_tcache.c \
	$(SRC_DIR)/hz4_mid.c \
	$(SRC_DIR)/hz4_large.c \
	$(SRC_DIR)/hz4_shim.c \
	$(SRC_DIR)/hz4_inbox.c \
	$(SRC_DIR)/hz4_pagetag.c \
	$(SRC_DIR)/hz4_central_pageheap.c \
	$(SRC_DIR)/hz4_xfer.c \
	$(SRC_DIR)/hz4_tcbox.c

$(LIB_TARGET): $(LIB_SRCS) $(CORE_HEADERS) $(CORE_INCS) $(INC_HEADERS) $(MID_INCS)
	$(CC) $(CFLAGS) $(TLS_CFLAGS) -fPIC -I$(CORE_DIR) -I$(INC_DIR) -shared -o $@ $(LIB_SRCS) -lpthread

bench: $(OUT_DIR)/bench_hz4
	@echo "=== Running hz4 bench ==="
	./$(OUT_DIR)/bench_hz4 4 1000000 90 64

# ============================================================================
# Clean
# ============================================================================
clean:
	rm -rf $(OUT_DIR) $(LIB_TARGET)
