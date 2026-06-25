.PHONY: all clean smoke smoke-mediumupper48 safety-stress preload preload-smoke bench bench-medium64k2 bench-mediumchunk bench-mediumupper48 bench-release bench-release-audit bench-release-upper1p5 bench-release-upper3072 bench-release-mediummadvfree bench-release-medium64k2 bench-release-mediumchunk bench-release-mediumupper48 preload-mediummadvfree medium-v1-gate medium-retention-closeout medium-retention-closeout-madvfree medium-chunk-paired-gate medium-sizepolicy-paired-gate medium-64k2-budget-paired-gate remote-micro remote-micro-release
.NOTPARALLEL: clean

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
LDFLAGS ?=
LDLIBS ?= -pthread -ldl
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src
DEBUG_CFLAGS := $(CFLAGS) -DH8_ENABLE_DEBUG_STATS
BENCH_ATTR_CFLAGS := -DH8_BENCH_ATTRIBUTION
MEDIUM_BUDGET_CFLAGS ?=
MEDIUM_COLLECT_CFLAGS ?=
SRC := $(wildcard $(ROOT)/src/*.c)
HEADERS := $(wildcard $(ROOT)/src/*.h) $(wildcard $(ROOT)/include/*.h)
CORE_SRC := $(filter-out $(ROOT)/src/h8_preload_boundary.c,$(SRC))
SMOKE_SRC := $(SRC) $(ROOT)/tests/h8_smoke.c
SAFETY_STRESS_SRC := $(SRC) $(ROOT)/tests/h8_safety_stress.c
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

bench: $(ROOT)/h8_bench

bench-medium64k2: $(ROOT)/h8_bench_medium64k2

bench-mediumchunk: $(ROOT)/h8_bench_mediumchunk

bench-mediumupper48: $(ROOT)/h8_bench_mediumupper48

bench-release: $(ROOT)/h8_bench_release

bench-release-audit: $(ROOT)/h8_bench_release_audit

bench-release-upper1p5: $(ROOT)/h8_bench_release_upper1p5

bench-release-upper3072: $(ROOT)/h8_bench_release_upper3072

bench-release-mediummadvfree: $(ROOT)/h8_bench_release_mediummadvfree

bench-release-medium64k2: $(ROOT)/h8_bench_release_medium64k2

bench-release-mediumchunk: $(ROOT)/h8_bench_release_mediumchunk

bench-release-mediumupper48: $(ROOT)/h8_bench_release_mediumupper48

preload-mediummadvfree: $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so

medium-v1-gate: $(ROOT)/h8_bench_release
	$(ROOT)/scripts/run_medium_v1_gate.sh

medium-retention-closeout:
	$(ROOT)/scripts/run_medium_retention_closeout.sh

medium-retention-closeout-madvfree: $(ROOT)/h8_bench_release_mediummadvfree $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so
	H8_SKIP_BUILD=1 BENCH_BIN=$(ROOT)/h8_bench_release_mediummadvfree PRELOAD_SO=$(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so $(ROOT)/scripts/run_medium_retention_closeout.sh

medium-chunk-paired-gate: $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_mediumchunk
	$(ROOT)/scripts/run_medium_chunk_paired_gate.sh

medium-sizepolicy-paired-gate: $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_mediumupper48
	$(ROOT)/scripts/run_medium_sizepolicy_paired_gate.sh

medium-64k2-budget-paired-gate:
	$(ROOT)/scripts/run_medium_64k2_budget_paired_gate.sh

remote-micro: $(ROOT)/h8_remote_micro

remote-micro-release: $(ROOT)/h8_remote_micro_release

$(ROOT)/libhakozuna_hz8_preload.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -DH8_MEDIUM_BUDGET_REJECT_MADV_FREE -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke_mediumupper48: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_safety_stress: $(SAFETY_STRESS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SAFETY_STRESS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_preload_smoke: $(PRELOAD_SMOKE_SRC)
	$(CC) $(CFLAGS) -o $@ $(PRELOAD_SMOKE_SRC) $(LDFLAGS) -ldl

$(ROOT)/h8_bench: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_medium64k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_64K_TWO_SLOT -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_mediumupper48: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(BENCH_ATTR_CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_audit: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(BENCH_ATTR_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_upper1p5: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_CLASS_MAP_UPPER1P5 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_upper3072: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -DH8_CLASS_MAP_UPPER3072 -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediummadvfree: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_BUDGET_REJECT_MADV_FREE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_medium64k2: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_64K_TWO_SLOT -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumchunk: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_CHUNK_CARVE -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_mediumupper48: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(MEDIUM_BUDGET_CFLAGS) $(INC) -DH8_MEDIUM_UPPER48_CLASS -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_remote_micro: $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_remote_micro_release: $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(REMOTE_MICRO_SRC) $(BENCH_SUPPORT_SRC) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(ROOT)/libhakozuna_hz8_preload.so $(ROOT)/libhakozuna_hz8_preload_mediummadvfree.so $(ROOT)/h8_smoke $(ROOT)/h8_smoke_mediumupper48 $(ROOT)/h8_safety_stress $(ROOT)/h8_preload_smoke $(ROOT)/h8_bench $(ROOT)/h8_bench_medium64k2 $(ROOT)/h8_bench_mediumchunk $(ROOT)/h8_bench_mediumupper48 $(ROOT)/h8_bench_release $(ROOT)/h8_bench_release_audit $(ROOT)/h8_bench_release_upper1p5 $(ROOT)/h8_bench_release_upper3072 $(ROOT)/h8_bench_release_mediummadvfree $(ROOT)/h8_bench_release_medium64k2 $(ROOT)/h8_bench_release_mediumchunk $(ROOT)/h8_bench_release_mediumupper48 $(ROOT)/h8_remote_micro $(ROOT)/h8_remote_micro_release
