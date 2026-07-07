.PHONY: all smoke bench-fixed-local preload preload-span preload-token-stats \
        preload-span-stats hz11-standalone-check clean

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
# Speed lane: LTO. A later tuning box may add -O3/-march=native; kept at -O2 for
# the first gate so the HZ10/tcmalloc comparison is apples-to-apples.
OPT_CFLAGS := $(CFLAGS) -flto
SHIM_CFLAGS := $(OPT_CFLAGS) -fno-builtin -ftls-model=initial-exec
SHIM_LDFLAGS := $(LDFLAGS) -Wl,-Bsymbolic-functions
LDLIBS ?= -pthread -ldl

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src

CORE_SRC := $(ROOT)/src/hz11_size_class.c $(ROOT)/src/hz11_thread_cache.c \
            $(ROOT)/src/hz11_public_entry.c $(ROOT)/src/hz11_span.c
SHIM_SRC := $(ROOT)/src/hz11_shim.c $(CORE_SRC)
HEADERS := $(wildcard $(ROOT)/src/*.h) $(wildcard $(ROOT)/include/*.h)

SMOKE_SRC := $(ROOT)/tests/hz11_thread_cache_smoke.c $(CORE_SRC)
BENCH_SRC := $(ROOT)/bench/hz11_fixed_local_bench.c

# Binary targets are relative so `make hz11_fixed_local_bench` works.
all: smoke preload preload-span preload-token-stats preload-span-stats

# --- smoke binaries (speed + stats, token + span) ---

hz11_thread_cache_smoke: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_stats: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_ENABLE_HOT_COUNTERS=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_stats: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_ENABLE_HOT_COUNTERS=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke: hz11_thread_cache_smoke hz11_thread_cache_smoke_span \
        hz11_thread_cache_smoke_stats hz11_thread_cache_smoke_span_stats
	./hz11_thread_cache_smoke
	./hz11_thread_cache_smoke_span
	./hz11_thread_cache_smoke_stats
	./hz11_thread_cache_smoke_span_stats

hz11_fixed_local_bench: $(BENCH_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-fixed-local: hz11_fixed_local_bench
	SLOT_SIZE=64 ITERS=20000000 ./hz11_fixed_local_bench

# --- preload siblings (speed lane: counters OFF; stats lane: counters ON) ---

libhz11.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) $(INC) -shared -Wl,-soname,libhz11.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 $(INC) -shared -Wl,-soname,libhz11_span.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# Stats siblings: hot-path counters ON (diagnostic).
libhz11_stats.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_ENABLE_HOT_COUNTERS=1 $(INC) -shared -Wl,-soname,libhz11_stats.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_stats.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_ENABLE_HOT_COUNTERS=1 $(INC) -shared -Wl,-soname,libhz11_span_stats.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload: libhz11.so
preload-span: libhz11_span.so
preload-token-stats: libhz11_stats.so
preload-span-stats: libhz11_span_stats.so

hz11-standalone-check:
	./scripts/check_hz11_standalone.sh

clean:
	rm -f hz11_thread_cache_smoke hz11_thread_cache_smoke_span \
	      hz11_thread_cache_smoke_stats hz11_thread_cache_smoke_span_stats \
	      hz11_fixed_local_bench libhz11.so libhz11_span.so \
	      libhz11_stats.so libhz11_span_stats.so
