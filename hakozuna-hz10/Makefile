.PHONY: all clean smoke-pagemap-route bench-pagemap-route hz10-standalone-check

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
LDFLAGS ?=
LDLIBS ?= -pthread
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src
DEBUG_CFLAGS := $(CFLAGS) -DHZ10_ENABLE_DEBUG_STATS
# -flto: without it, hz10_pagemap_route() (a real cross-TU call from
# bench/hz10_pagemap_route_bench.c into src/hz10_pagemap.c) never gets
# inlined the way the in-process hash-probe baseline (defined statically in
# the same bench TU) does -- that asymmetry alone was worth ~30% in earlier
# measurement, in the hash baseline's favor, for reasons that have nothing
# to do with either lookup algorithm. LTO removes that bias so the bench
# compares the two mechanisms on equal footing.
BENCH_CFLAGS := $(CFLAGS) -flto

HEADERS := $(wildcard $(ROOT)/src/*.h) $(wildcard $(ROOT)/include/*.h)
PAGEMAP_SRC := $(ROOT)/src/hz10_pagemap.c $(ROOT)/src/hz10_platform.c
SMOKE_PAGEMAP_SRC := $(ROOT)/tests/hz10_pagemap_route_smoke.c $(PAGEMAP_SRC)
BENCH_PAGEMAP_SRC := $(ROOT)/bench/hz10_pagemap_route_bench.c $(PAGEMAP_SRC)

all: smoke-pagemap-route bench-pagemap-route

$(ROOT)/hz10_pagemap_route_smoke: $(SMOKE_PAGEMAP_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_PAGEMAP_SRC) $(LDFLAGS) $(LDLIBS)

smoke-pagemap-route: $(ROOT)/hz10_pagemap_route_smoke
	$(ROOT)/hz10_pagemap_route_smoke

$(ROOT)/hz10_pagemap_route_bench: $(BENCH_PAGEMAP_SRC) $(HEADERS)
	$(CC) $(BENCH_CFLAGS) $(INC) -o $@ $(BENCH_PAGEMAP_SRC) $(LDFLAGS) $(LDLIBS)

bench-pagemap-route: $(ROOT)/hz10_pagemap_route_bench
	$(ROOT)/hz10_pagemap_route_bench

hz10-standalone-check:
	$(ROOT)/scripts/check_hz10_standalone.sh

clean:
	rm -f $(ROOT)/hz10_pagemap_route_smoke $(ROOT)/hz10_pagemap_route_bench
