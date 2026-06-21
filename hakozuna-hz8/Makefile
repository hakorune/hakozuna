.PHONY: all clean smoke preload bench bench-release
.NOTPARALLEL: clean

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
LDFLAGS ?=
LDLIBS ?= -pthread -ldl
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src
DEBUG_CFLAGS := $(CFLAGS) -DH8_ENABLE_DEBUG_STATS
SRC := $(wildcard $(ROOT)/src/*.c)
CORE_SRC := $(filter-out $(ROOT)/src/h8_preload_boundary.c,$(SRC))
SMOKE_SRC := $(SRC) $(ROOT)/tests/h8_smoke.c

all: smoke preload bench

preload: $(ROOT)/libhakozuna_hz8_preload.so

smoke: $(ROOT)/h8_smoke

bench: $(ROOT)/h8_bench

bench-release: $(ROOT)/h8_bench_release

$(ROOT)/libhakozuna_hz8_preload.so: $(SRC)
	$(CC) $(CFLAGS) $(INC) -DH8_BUILD_LD_PRELOAD -shared -o $@ $(SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_smoke: $(SMOKE_SRC)
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench: $(SRC) $(ROOT)/bench/h8_bench.c
	$(CC) $(DEBUG_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release: $(SRC) $(ROOT)/bench/h8_bench.c
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(ROOT)/libhakozuna_hz8_preload.so $(ROOT)/h8_smoke $(ROOT)/h8_bench $(ROOT)/h8_bench_release
