.PHONY: all clean smoke smoke-mediumupper48 safety-stress medium-lazy-saturation preload preload-smoke bench bench-medium64k2 bench-mediumchunk bench-mediumshardchunk bench-mediumupper48 bench-mediumfreecache bench-release bench-release-audit bench-release-upper1p5 bench-release-upper3072 bench-release-mediumnolazy bench-release-mediumfreecache bench-release-mediummadvfree bench-release-mediumlazy bench-release-medium64k2 bench-release-mediumchunk bench-release-mediumshardchunk bench-release-mediumupper48 preload-mediumnolazy preload-mediumchunk preload-mediummadvfree preload-mediumlazy medium-v1-gate medium-retention-closeout medium-retention-closeout-chunk medium-retention-closeout-madvfree medium-retention-closeout-lazy medium-chunk-paired-gate medium-shardchunk-paired-gate medium-lazy-paired-gate medium-sizepolicy-paired-gate medium-64k2-budget-paired-gate remote-micro remote-micro-release
.NOTPARALLEL: clean

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
LDFLAGS ?=
LDLIBS ?= -pthread -ldl
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src
DEBUG_CFLAGS := $(CFLAGS) -DH8_ENABLE_DEBUG_STATS
BENCH_ATTR_CFLAGS := -DH8_BENCH_ATTRIBUTION
MEDIUM_BUDGET_CFLAGS ?= -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE
MEDIUM_COLLECT_CFLAGS ?=
SRC := $(wildcard $(ROOT)/src/*.c)
HEADERS := $(wildcard $(ROOT)/src/*.h) $(wildcard $(ROOT)/include/*.h)
CORE_SRC := $(filter-out $(ROOT)/src/h8_preload_boundary.c,$(SRC))
SMOKE_SRC := $(SRC) $(ROOT)/tests/h8_smoke.c
SAFETY_STRESS_SRC := $(SRC) $(ROOT)/tests/h8_safety_stress.c
MEDIUM_LAZY_SATURATION_SRC := $(SRC) $(ROOT)/tests/h8_medium_lazy_saturation.c
PRELOAD_SMOKE_SRC := $(ROOT)/tests/h8_preload_smoke.c
BENCH_SUPPORT_SRC := $(ROOT)/bench/h8_bench_support.c
BENCH_REPORT_SRC := $(ROOT)/bench/h8_bench_report.c
BENCH_WORKERS_SRC := $(ROOT)/bench/h8_bench_workers.c
REMOTE_MICRO_SRC := $(ROOT)/bench/h8_remote_micro.c

all: smoke preload bench

preload: $(ROOT)/libhakozuna_hz8_preload.so

preload-smoke: $(ROOT)/h8_preload_smoke $(ROOT)/libhakozuna_hz8_preload.so
	LD_PRELOAD=$(ROOT)/libhakozuna_hz8_preload.so $(ROOT)/h8_preload_smoke

smoke: $(ROOT)/h8_smoke

smoke-mediumupper48: $(ROOT)/h8_smoke_mediumupper48

safety-stress: $(ROOT)/h8_safety_stress

medium-lazy-saturation: $(ROOT)/h8_medium_lazy_saturation
	$(ROOT)/h8_medium_lazy_saturation

bench: $(ROOT)/h8_bench

bench-medium64k2: $(ROOT)/h8_bench_medium64k2

bench-mediumchunk: $(ROOT)/h8_bench_mediumchunk

bench-mediumshardchunk: $(ROOT)/h8_bench_mediumshardchunk

bench-mediumupper48: $(ROOT)/h8_bench_mediumupper48

bench-mediumfreecache: $(ROOT)/h8_bench_mediumfreecache

bench-release: $(ROOT)/h8_bench_release

bench-release-audit: $(ROOT)/h8_bench_release_audit

bench-release-upper1p5: $(ROOT)/h8_bench_release_upper1p5

bench-release-upper3072: $(ROOT)/h8_bench_release_upper3072

bench-release-mediumnolazy: $(ROOT)/h8_bench_release_mediumnolazy

bench-release-mediumfreecache: $(ROOT)/h8_bench_release_mediumfreecache

bench-release-mediummadvfree: $(ROOT)/h8_bench_release_mediummadvfree

bench-release-mediumlazy: $(ROOT)/h8_bench_release_mediumlazy

bench-release-medium64k2: $(ROOT)/h8_bench_release_medium64k2

bench-release-mediumchunk: $(ROOT)/h8_bench_release_mediumchunk

bench-release-mediumshardchunk: $(ROOT)/h8_bench_release_mediumshardchunk

bench-release-mediumupper48: $(ROOT)/h8_bench_release_mediumupper48

preload-mediumnolazy: $(ROOT)/libhakozuna_hz8_preload_mediumnolazy.so

preload-mediumchunk: $(ROOT)/libhakozuna_hz8_preload_mediumchunk.so

preload-mediummadvfree: $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so

preload-mediumlazy: $(ROOT)/libhakozuna_hz8_preload_mediumlazy.so

medium-v1-gate: $(ROOT)/h8_bench_release
	$(ROOT)/scripts/run_medium_v1_gate.sh

medium-retention-closeout:
	$(ROOT)/scripts/run_medium_retention_closeout.sh

medium-retention-closeout-chunk: $(ROOT)/h8_bench_release_mediumchunk $(ROOT)/libhakozuna_hz8_preload_mediumchunk.so
	H8_SKIP_BUILD=1 BENCH_BIN=$(ROOT)/h8_bench_release_mediumchunk PRELOAD_SO=$(ROOT)/libhakozuna_hz8_preload_mediumchunk.so $(ROOT)/scripts/run_medium_retention_closeout.sh

medium-retention-closeout-madvfree: $(ROOT)/h8_bench_release_mediummadvfree $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so
	H8_SKIP_BUILD=1 BENCH_BIN=$(ROOT)/h8_bench_release_mediummadvfree PRELOAD_SO=$(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so $(ROOT)/scripts/run_medium_retention_closeout.sh

medium-retention-closeout-lazy: $(ROOT)/h8_bench_release_mediumlazy $(ROOT)/libhakozuna_hz8_preload_mediumlazy.so
	H8_SKIP_BUILD=1 BENCH_BIN=$(ROOT)/h8_bench_release_mediumlazy PRELOAD_SO=$(ROOT)/libhakozuna_hz8_preload_mediumlazy.so $(ROOT)/scripts/run_medium_retention_closeout.sh

medium-chunk-paired-gate: $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_mediumchunk
	$(ROOT)/scripts/run_medium_chunk_paired_gate.sh

medium-shardchunk-paired-gate: $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_mediumshardchunk
	CANDIDATE_BIN=$(ROOT)/h8_bench_release_mediumshardchunk CANDIDATE_NAME=shardchunk $(ROOT)/scripts/run_medium_chunk_paired_gate.sh

medium-lazy-paired-gate: $(ROOT)/h8_bench_release_mediumnolazy $(ROOT)/h8_bench_release
	FRESH_PROCESS=1 BASELINE_BIN=$(ROOT)/h8_bench_release_mediumnolazy BASELINE_NAME=nolazy CANDIDATE_BIN=$(ROOT)/h8_bench_release CANDIDATE_NAME=lazy128 $(ROOT)/scripts/run_medium_chunk_paired_gate.sh

medium-sizepolicy-paired-gate: $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_mediumupper48
	$(ROOT)/scripts/run_medium_sizepolicy_paired_gate.sh

medium-64k2-budget-paired-gate:
	$(ROOT)/scripts/run_medium_64k2_budget_paired_gate.sh

remote-micro: $(ROOT)/h8_remote_micro

remote-micro-release: $(ROOT)/h8_remote_micro_release

$(ROOT)/libhakozuna_hz8_preload.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediumnolazy.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediumchunk.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_CHUNK_CARVE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_BUDGET_REJECT_MADV_FREE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediumlazy.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke_mediumupper48: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_safety_stress: $(SAFETY_STRESS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -o $@ $(SAFETY_STRESS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_medium_lazy_saturation: $(MEDIUM_LAZY_SATURATION_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -o $@ $(MEDIUM_LAZY_SATURATION_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_preload_smoke: $(PRELOAD_SMOKE_SRC)
	$(CC) $(CFLAGS) -o $@ $(PRELOAD_SMOKE_SRC) $(LDFLAGS) -ldl

$(ROOT)/h8_bench: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_medium64k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_64K_TWO_SLOT -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumshardchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_SHARDED_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumupper48: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumfreecache: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_LOCAL_FREE_CACHE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_audit: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_upper1p5: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_CLASS_MAP_UPPER1P5 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_upper3072: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_CLASS_MAP_UPPER3072 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumnolazy: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumfreecache: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_LOCAL_FREE_CACHE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediummadvfree: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_MEDIUM_BUDGET_REJECT_MADV_FREE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumlazy: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_medium64k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_64K_TWO_SLOT -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumshardchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_SHARDED_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumupper48: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_remote_micro: $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_remote_micro_release: $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(ROOT)/libhakozuna_hz8_preload.so $(ROOT)/libhakozuna_hz8_preload_mediumnolazy.so $(ROOT)/libhakozuna_hz8_preload_mediumchunk.so $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so $(ROOT)/libhakozuna_hz8_preload_mediumlazy.so $(ROOT)/h8_smoke $(ROOT)/h8_smoke_mediumupper48 $(ROOT)/h8_safety_stress $(ROOT)/h8_medium_lazy_saturation $(ROOT)/h8_preload_smoke $(ROOT)/h8_bench $(ROOT)/h8_bench_medium64k2 $(ROOT)/h8_bench_mediumchunk $(ROOT)/h8_bench_mediumshardchunk $(ROOT)/h8_bench_mediumupper48 $(ROOT)/h8_bench_mediumfreecache $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_audit $(ROOT)/h8_bench_release_upper1p5 $(ROOT)/h8_bench_release_upper3072 $(ROOT)/h8_bench_release_mediumnolazy $(ROOT)/h8_bench_release_mediumfreecache $(ROOT)/h8_bench_release_mediummadvfree $(ROOT)/h8_bench_release_mediumlazy $(ROOT)/h8_bench_release_medium64k2 $(ROOT)/h8_bench_release_mediumchunk $(ROOT)/h8_bench_release_mediumshardchunk $(ROOT)/h8_bench_release_mediumupper48 $(ROOT)/h8_remote_micro $(ROOT)/h8_remote_micro_release
