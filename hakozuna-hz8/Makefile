.PHONY: all clean smoke smoke-mediumupper48 safety-stress medium-lazy-saturation preload preload-smoke bench bench-activefulldefer8 bench-defer4mediumcapacity bench-defer8mediumcapacity bench-remoteactivefullbudget bench-mediumcapacitybudget bench-medium64k2 bench-mediumchunk bench-mediumshardchunk bench-mediumupper48 bench-mediumv12_48k2 bench-mediumfreecache bench-mediumrefillhint bench-mediumrefillcandidate bench-mediumavailable bench-mediumdemand64 bench-mediumlocalfasttier bench-hz9mediumlocalmagshadow bench-release bench-release-audit bench-release-upper1p5 bench-release-upper3072 bench-release-mediumnolazy bench-release-mediumfreecache bench-release-mediumrefillhint bench-release-mediumrefillcandidate bench-release-mediumavailable bench-release-mediumavailableinline bench-release-mediumdemand64 bench-release-mediumlocalfasttier bench-release-hz9mediumlocalmagshadow bench-release-mediumceiling-noslotstate bench-release-mediumceiling-freepending bench-release-mediumceiling-combined bench-release-mediummadvfree bench-release-mediumlazy bench-release-medium64k2 bench-release-mediumchunk bench-release-mediumshardchunk bench-release-mediumupper48 bench-release-mediumv12_48k2 preload-mediumnolazy preload-medium64k2 preload-mediumchunk preload-mediummadvfree preload-mediumlazy medium-v1-gate medium-retention-closeout medium-retention-closeout-chunk medium-retention-closeout-madvfree medium-retention-closeout-lazy medium-chunk-paired-gate medium-shardchunk-paired-gate medium-lazy-paired-gate medium-sizepolicy-paired-gate medium-64k2-budget-paired-gate remote-micro remote-micro-release
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

smoke-largedirect: $(ROOT)/h8_smoke_largedirect

smoke-mediumupper48: $(ROOT)/h8_smoke_mediumupper48

safety-stress: $(ROOT)/h8_safety_stress

medium-lazy-saturation: $(ROOT)/h8_medium_lazy_saturation
	$(ROOT)/h8_medium_lazy_saturation

bench: $(ROOT)/h8_bench

bench-activefulldefer8: $(ROOT)/h8_bench_activefulldefer8

bench-defer4mediumcapacity: $(ROOT)/h8_bench_defer4mediumcapacity

bench-defer4mediumcapacity-largedirect: $(ROOT)/h8_bench_defer4mediumcapacity_largedirect

bench-defer8mediumcapacity: $(ROOT)/h8_bench_defer8mediumcapacity

bench-remoteactivefullbudget: $(ROOT)/h8_bench_remoteactivefullbudget

bench-mediumcapacitybudget: $(ROOT)/h8_bench_mediumcapacitybudget

bench-medium64k2: $(ROOT)/h8_bench_medium64k2

bench-mediumchunk: $(ROOT)/h8_bench_mediumchunk

bench-mediumshardchunk: $(ROOT)/h8_bench_mediumshardchunk

bench-mediumupper48: $(ROOT)/h8_bench_mediumupper48

bench-mediumv12_48k2: $(ROOT)/h8_bench_mediumv12_48k2

bench-mediumfreecache: $(ROOT)/h8_bench_mediumfreecache

bench-mediumrefillhint: $(ROOT)/h8_bench_mediumrefillhint

bench-mediumrefillcandidate: $(ROOT)/h8_bench_mediumrefillcandidate

bench-mediumavailable: $(ROOT)/h8_bench_mediumavailable

bench-mediumdemand64: $(ROOT)/h8_bench_mediumdemand64

bench-mediumlocalfasttier: $(ROOT)/h8_bench_mediumlocalfasttier

bench-hz9mediumlocalmagshadow: $(ROOT)/h8_bench_hz9mediumlocalmagshadow

bench-release: $(ROOT)/h8_bench_release

bench-release-audit: $(ROOT)/h8_bench_release_audit

bench-release-upper1p5: $(ROOT)/h8_bench_release_upper1p5

bench-release-upper3072: $(ROOT)/h8_bench_release_upper3072

bench-release-mediumnolazy: $(ROOT)/h8_bench_release_mediumnolazy

bench-release-mediumfreecache: $(ROOT)/h8_bench_release_mediumfreecache

bench-release-mediumrefillhint: $(ROOT)/h8_bench_release_mediumrefillhint

bench-release-mediumrefillcandidate: $(ROOT)/h8_bench_release_mediumrefillcandidate

bench-release-mediumavailable: $(ROOT)/h8_bench_release_mediumavailable

bench-release-mediumavailableinline: $(ROOT)/h8_bench_release_mediumavailableinline

bench-release-mediumdemand64: $(ROOT)/h8_bench_release_mediumdemand64

bench-release-mediumlocalfasttier: $(ROOT)/h8_bench_release_mediumlocalfasttier

bench-release-hz9mediumlocalmagshadow: $(ROOT)/h8_bench_release_hz9mediumlocalmagshadow

bench-release-mediumceiling-noslotstate: $(ROOT)/h8_bench_release_mediumceiling_noslotstate

bench-release-mediumceiling-freepending: $(ROOT)/h8_bench_release_mediumceiling_freepending

bench-release-mediumceiling-combined: $(ROOT)/h8_bench_release_mediumceiling_combined

bench-release-mediummadvfree: $(ROOT)/h8_bench_release_mediummadvfree

bench-release-mediumlazy: $(ROOT)/h8_bench_release_mediumlazy

bench-release-medium64k2: $(ROOT)/h8_bench_release_medium64k2

bench-release-mediumchunk: $(ROOT)/h8_bench_release_mediumchunk

bench-release-mediumshardchunk: $(ROOT)/h8_bench_release_mediumshardchunk

bench-release-mediumupper48: $(ROOT)/h8_bench_release_mediumupper48

bench-release-mediumv12_48k2: $(ROOT)/h8_bench_release_mediumv12_48k2

preload-mediumnolazy: $(ROOT)/libhakozuna_hz8_preload_mediumnolazy.so

preload-medium64k2: $(ROOT)/libhakozuna_hz8_preload_medium64k2.so

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

$(ROOT)/libhakozuna_hz8_preload_medium64k2.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_LEGACY_Q64_RUN64K2 -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediumchunk.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_CHUNK_CARVE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_BUDGET_REJECT_MADV_FREE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediumlazy.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke_largedirect: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_LARGE_DIRECT_OWNED_L1 -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

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

$(ROOT)/h8_bench_activefulldefer8: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_defer4mediumcapacity: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_defer4mediumcapacity_largedirect: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_LIMIT=4 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -DH8_LARGE_DIRECT_OWNED_L1 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_defer8mediumcapacity: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -DH8_REMOTE_PRESSURE_ACTIVE_FULL_DEFER_L1 -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_remoteactivefullbudget: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -DH8_REMOTE_PRESSURE_ACTIVE_FULL_BUDGET_L1 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumcapacitybudget: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -DH8_MEDIUM_CAPACITY_COLLECT_BUDGET_L1 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_medium64k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_LEGACY_Q64_RUN64K2 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumshardchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_SHARDED_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumupper48: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumv12_48k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_V12_48K2_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumfreecache: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_LOCAL_FREE_CACHE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumrefillhint: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_COLLECT_ACTIVE_REFILL_HINT -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumrefillcandidate: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_REFILL_CANDIDATE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumavailable: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_AVAILABLE_INDEX -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumdemand64: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_ACTIVE_MISS_DEMAND_COLLECT_64K -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumlocalfasttier: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_LOCAL_FAST_TIER -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_hz9mediumlocalmagshadow: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_HZ9_MEDIUM_LOCAL_MAG_SHADOW -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

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

$(ROOT)/h8_bench_release_mediumrefillhint: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_COLLECT_ACTIVE_REFILL_HINT -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumrefillcandidate: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_REFILL_CANDIDATE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumavailable: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_AVAILABLE_INDEX -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumavailableinline: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_AVAILABLE_INDEX -DH8_MEDIUM_ENABLE_INLINE_OWNER_ALLOC -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumdemand64: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_ACTIVE_MISS_DEMAND_COLLECT_64K -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumlocalfasttier: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_ENABLE_LOCAL_FAST_TIER -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_hz9mediumlocalmagshadow: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_HZ9_MEDIUM_LOCAL_MAG_SHADOW -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumceiling_noslotstate: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumceiling_freepending: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_LOCAL_FREE_PENDING_ELIDE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumceiling_combined: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CEILING_ALLOC_NO_SLOT_STATE -DH8_MEDIUM_LOCAL_FREE_PENDING_ELIDE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediummadvfree: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_MEDIUM_BUDGET_REJECT_MADV_FREE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumlazy: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_BUDGET_REJECT_LAZY_PURGE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_medium64k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_LEGACY_Q64_RUN64K2 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumshardchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_SHARDED_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumupper48: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumv12_48k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_V12_48K2_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_remote_micro: $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_remote_micro_release: $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(ROOT)/libhakozuna_hz8_preload.so $(ROOT)/libhakozuna_hz8_preload_mediumnolazy.so $(ROOT)/libhakozuna_hz8_preload_medium64k2.so $(ROOT)/libhakozuna_hz8_preload_mediumchunk.so $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so $(ROOT)/libhakozuna_hz8_preload_mediumlazy.so $(ROOT)/h8_smoke $(ROOT)/h8_smoke_mediumupper48 $(ROOT)/h8_safety_stress $(ROOT)/h8_medium_lazy_saturation $(ROOT)/h8_preload_smoke $(ROOT)/h8_bench $(ROOT)/h8_bench_medium64k2 $(ROOT)/h8_bench_mediumchunk $(ROOT)/h8_bench_mediumshardchunk $(ROOT)/h8_bench_mediumupper48 $(ROOT)/h8_bench_mediumv12_48k2 $(ROOT)/h8_bench_mediumfreecache $(ROOT)/h8_bench_mediumrefillhint $(ROOT)/h8_bench_mediumrefillcandidate $(ROOT)/h8_bench_mediumavailable $(ROOT)/h8_bench_mediumdemand64 $(ROOT)/h8_bench_mediumlocalfasttier $(ROOT)/h8_bench_hz9mediumlocalmagshadow $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_audit $(ROOT)/h8_bench_release_upper1p5 $(ROOT)/h8_bench_release_upper3072 $(ROOT)/h8_bench_release_mediumnolazy $(ROOT)/h8_bench_release_mediumfreecache $(ROOT)/h8_bench_release_mediumrefillhint $(ROOT)/h8_bench_release_mediumrefillcandidate $(ROOT)/h8_bench_release_mediumavailable $(ROOT)/h8_bench_release_mediumavailableinline $(ROOT)/h8_bench_release_mediumdemand64 $(ROOT)/h8_bench_release_mediumlocalfasttier $(ROOT)/h8_bench_release_hz9mediumlocalmagshadow $(ROOT)/h8_bench_release_mediumceiling_noslotstate $(ROOT)/h8_bench_release_mediumceiling_freepending $(ROOT)/h8_bench_release_mediumceiling_combined $(ROOT)/h8_bench_release_mediummadvfree $(ROOT)/h8_bench_release_mediumlazy $(ROOT)/h8_bench_release_medium64k2 $(ROOT)/h8_bench_release_mediumchunk $(ROOT)/h8_bench_release_mediumshardchunk $(ROOT)/h8_bench_release_mediumupper48 $(ROOT)/h8_bench_release_mediumv12_48k2 $(ROOT)/h8_remote_micro $(ROOT)/h8_remote_micro_release
