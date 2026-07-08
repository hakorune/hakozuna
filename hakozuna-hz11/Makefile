.PHONY: all smoke bench-fixed-local preload preload-span preload-token-stats \
        preload-span-stats preload-token-top preload-span-top \
        preload-token-tlsfast preload-span-tlsfast \
        preload-token-nobytes preload-span-nobytes \
        preload-token-soa preload-span-soa \
        preload-span-transfer preload-span-transfer-diag preload-span-transfer-cap \
        preload-span-return preload-span-source-diag preload-span-return-source-diag \
        preload-span-transfer-thread-exit preload-span-transfer-thread-exit-cap \
        preload-span-transfer-thread-exit-cap-source-diag \
        preload-span-transfer-thread-exit-cap-batch32 \
        preload-span-transfer-thread-exit-cap-batch32-source-diag \
        preload-span-transfer-thread-exit-cap-batch64 \
        preload-span-transfer-thread-exit-cap-batch128 \
        preload-span-transfer-thread-exit-cap-cap8192 \
        preload-span-transfer-thread-exit-cap-xferwide \
        hz11-standalone-check clean

CC ?= gcc
CFLAGS ?= -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE
# Speed lane: LTO. A later tuning box may add -O3/-march=native; kept at -O2 for
# the first gate so the HZ10/tcmalloc comparison is apples-to-apples.
OPT_CFLAGS := $(CFLAGS) -flto
SHIM_CFLAGS := $(OPT_CFLAGS) -fno-builtin -ftls-model=initial-exec
SHIM_LDFLAGS := $(LDFLAGS) -Wl,-Bsymbolic-functions
LDLIBS ?= -pthread -ldl
HZ11_CENTRAL_CAP_AB ?= 32768

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INC := -I$(ROOT)/include -I$(ROOT)/src

CORE_SRC := $(ROOT)/src/hz11_size_class.c $(ROOT)/src/hz11_thread_cache.c \
            $(ROOT)/src/hz11_public_entry.c $(ROOT)/src/hz11_span.c \
            $(ROOT)/src/hz11_sys_alloc.c $(ROOT)/src/hz11_transfer_cache.c
SHIM_SRC := $(ROOT)/src/hz11_shim.c $(CORE_SRC)
HEADERS := $(wildcard $(ROOT)/src/*.h) $(wildcard $(ROOT)/include/*.h)

SMOKE_SRC := $(ROOT)/tests/hz11_thread_cache_smoke.c $(CORE_SRC)
BENCH_SRC := $(ROOT)/bench/hz11_fixed_local_bench.c

# Binary targets are relative so `make hz11_fixed_local_bench` works.
all: smoke preload preload-span preload-token-stats preload-span-stats \
     preload-token-top preload-span-top preload-token-tlsfast preload-span-tlsfast \
     preload-token-nobytes preload-span-nobytes \
     preload-token-soa preload-span-soa

# --- smoke binaries (speed + stats, token + span) ---

hz11_thread_cache_smoke: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_stats: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_ENABLE_HOT_COUNTERS=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_stats: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_ENABLE_HOT_COUNTERS=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_top: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CACHE_TOPPTR=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_top: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_CACHE_TOPPTR=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_tlsfast: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_TLS_FASTPATH=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_tlsfast: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_nobytes: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_nobytes: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_soa: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_soa: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_transfer: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

hz11_thread_cache_smoke_span_return: $(SMOKE_SRC) $(HEADERS)
	$(CC) $(OPT_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CENTRAL_SPAN_RETURN=1 -DHZ11_CENTRAL_CAP=65536 $(INC) -o $@ $(SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke: hz11_thread_cache_smoke hz11_thread_cache_smoke_span \
        hz11_thread_cache_smoke_stats hz11_thread_cache_smoke_span_stats \
        hz11_thread_cache_smoke_top hz11_thread_cache_smoke_span_top \
        hz11_thread_cache_smoke_tlsfast hz11_thread_cache_smoke_span_tlsfast \
        hz11_thread_cache_smoke_nobytes hz11_thread_cache_smoke_span_nobytes \
        hz11_thread_cache_smoke_soa hz11_thread_cache_smoke_span_soa \
        hz11_thread_cache_smoke_span_transfer hz11_thread_cache_smoke_span_return
	./hz11_thread_cache_smoke
	./hz11_thread_cache_smoke_span
	./hz11_thread_cache_smoke_stats
	./hz11_thread_cache_smoke_span_stats
	./hz11_thread_cache_smoke_top
	./hz11_thread_cache_smoke_span_top
	./hz11_thread_cache_smoke_tlsfast
	./hz11_thread_cache_smoke_span_tlsfast
	./hz11_thread_cache_smoke_nobytes
	./hz11_thread_cache_smoke_span_nobytes
	./hz11_thread_cache_smoke_soa
	./hz11_thread_cache_smoke_span_soa
	./hz11_thread_cache_smoke_span_transfer
	./hz11_thread_cache_smoke_span_return

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

# HZ11CacheShape-L1: pointer-top pop/push (A/B vs count-indexed).
libhz11_top.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CACHE_TOPPTR=1 $(INC) -shared -Wl,-soname,libhz11_top.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_top.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_CACHE_TOPPTR=1 $(INC) -shared -Wl,-soname,libhz11_span_top.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11TLSFastPath-L1: public-entry TLS-present fast/slow split.
libhz11_tlsfast.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_TLS_FASTPATH=1 $(INC) -shared -Wl,-soname,libhz11_tlsfast.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_tlsfast.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 $(INC) -shared -Wl,-soname,libhz11_span_tlsfast.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11CacheByteAccountingGate-L1: TLS fast path + no global cached-byte cap.
libhz11_nobytes.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 $(INC) -shared -Wl,-soname,libhz11_nobytes.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_nobytes.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 $(INC) -shared -Wl,-soname,libhz11_span_nobytes.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11CacheLayout-L1: SOA class cache (speed-ceiling lane).
libhz11_soa.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 $(INC) -shared -Wl,-soname,libhz11_soa.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_soa.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 $(INC) -shared -Wl,-soname,libhz11_span_soa.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11TransferCacheCentralSpan-L1: batch transfer cache + central span source.
libhz11_span_transfer.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 $(INC) -shared -Wl,-soname,libhz11_span_transfer.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11CurrentSpanPoolThreadExit-L1: opt-in current-span suffix salvage.
libhz11_span_transfer_thread_exit.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11CentralPolicyCorrectness-L1: focused correctness sibling only.
libhz11_span_transfer_thread_exit_cap.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11Sh6benchPathCostAttribution-L1: cap candidate with source counters.
libhz11_span_transfer_thread_exit_cap_source_diag.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_SPAN_SOURCE_DIAG=1 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_source_diag.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11Sh6benchTransferCentralPathCost-L1: transfer/central volume probe.
libhz11_span_transfer_thread_exit_cap_batch32.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_TRANSFER_BATCH=32 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_batch32.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_transfer_thread_exit_cap_batch32_source_diag.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_TRANSFER_BATCH=32 -DHZ11_SPAN_SOURCE_DIAG=1 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_batch32_source_diag.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_transfer_thread_exit_cap_batch64.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_TRANSFER_BATCH=64 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_batch64.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_transfer_thread_exit_cap_batch128.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_TRANSFER_BATCH=128 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_batch128.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_transfer_thread_exit_cap_cap8192.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_TRANSFER_CAP=8192 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_cap8192.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_transfer_thread_exit_cap_xferwide.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CURRENT_SPAN_THREAD_EXIT=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_TRANSFER_CAP=8192 -DHZ11_TRANSFER_BATCH=32 $(INC) -shared -Wl,-soname,libhz11_span_transfer_thread_exit_cap_xferwide.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11MacroFailureAttribution-L1: diagnostic sibling. Class counters are only
# compiled into this opt-in lane; the recommended transfer lane is unchanged.
libhz11_span_transfer_diag.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CENTRAL_CLASS_DIAG=1 $(INC) -shared -Wl,-soname,libhz11_span_transfer_diag.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_transfer_cap.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CENTRAL_CLASS_DIAG=1 -DHZ11_CENTRAL_CAP=$(HZ11_CENTRAL_CAP_AB) $(INC) -shared -Wl,-soname,libhz11_span_transfer_cap.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11CentralFreeListSpanReturn-L1: opt-in span-aware central free list.
libhz11_span_return.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CENTRAL_SPAN_RETURN=1 -DHZ11_CENTRAL_CAP=65536 $(INC) -shared -Wl,-soname,libhz11_span_return.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

# HZ11SpanSourceAttribution-L1: source counters only; no production lane.
libhz11_span_source_diag.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_SPAN_SOURCE_DIAG=1 $(INC) -shared -Wl,-soname,libhz11_span_source_diag.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

libhz11_span_return_source_diag.so: $(SHIM_SRC) $(HEADERS)
	$(CC) $(SHIM_CFLAGS) -DHZ11_CLASSIFY_SPAN=1 -DHZ11_TLS_FASTPATH=1 -DHZ11_CACHE_BYTE_ACCOUNTING=0 -DHZ11_CACHE_SOA=1 -DHZ11_TRANSFER_CENTRAL_SPAN=1 -DHZ11_CENTRAL_SPAN_RETURN=1 -DHZ11_CENTRAL_CAP=65536 -DHZ11_SPAN_SOURCE_DIAG=1 $(INC) -shared -Wl,-soname,libhz11_span_return_source_diag.so -o $@ $(SHIM_SRC) $(SHIM_LDFLAGS) $(LDLIBS)

preload: libhz11.so
preload-span: libhz11_span.so
preload-token-stats: libhz11_stats.so
preload-span-stats: libhz11_span_stats.so
preload-token-top: libhz11_top.so
preload-span-top: libhz11_span_top.so
preload-token-tlsfast: libhz11_tlsfast.so
preload-span-tlsfast: libhz11_span_tlsfast.so
preload-token-nobytes: libhz11_nobytes.so
preload-span-nobytes: libhz11_span_nobytes.so
preload-token-soa: libhz11_soa.so
preload-span-soa: libhz11_span_soa.so
preload-span-transfer: libhz11_span_transfer.so
preload-span-transfer-thread-exit: libhz11_span_transfer_thread_exit.so
preload-span-transfer-thread-exit-cap: libhz11_span_transfer_thread_exit_cap.so
preload-span-transfer-thread-exit-cap-source-diag: libhz11_span_transfer_thread_exit_cap_source_diag.so
preload-span-transfer-thread-exit-cap-batch32: libhz11_span_transfer_thread_exit_cap_batch32.so
preload-span-transfer-thread-exit-cap-batch32-source-diag: libhz11_span_transfer_thread_exit_cap_batch32_source_diag.so
preload-span-transfer-thread-exit-cap-batch64: libhz11_span_transfer_thread_exit_cap_batch64.so
preload-span-transfer-thread-exit-cap-batch128: libhz11_span_transfer_thread_exit_cap_batch128.so
preload-span-transfer-thread-exit-cap-cap8192: libhz11_span_transfer_thread_exit_cap_cap8192.so
preload-span-transfer-thread-exit-cap-xferwide: libhz11_span_transfer_thread_exit_cap_xferwide.so
preload-span-transfer-diag: libhz11_span_transfer_diag.so
preload-span-transfer-cap: libhz11_span_transfer_cap.so
preload-span-return: libhz11_span_return.so
preload-span-source-diag: libhz11_span_source_diag.so
preload-span-return-source-diag: libhz11_span_return_source_diag.so

hz11-standalone-check:
	./scripts/check_hz11_standalone.sh

clean:
	rm -f hz11_thread_cache_smoke hz11_thread_cache_smoke_span \
	      hz11_thread_cache_smoke_stats hz11_thread_cache_smoke_span_stats \
	      hz11_thread_cache_smoke_top hz11_thread_cache_smoke_span_top \
	      hz11_thread_cache_smoke_tlsfast hz11_thread_cache_smoke_span_tlsfast \
	      hz11_thread_cache_smoke_nobytes hz11_thread_cache_smoke_span_nobytes \
	      hz11_thread_cache_smoke_soa hz11_thread_cache_smoke_span_soa \
	      hz11_thread_cache_smoke_span_transfer hz11_thread_cache_smoke_span_return \
	      hz11_fixed_local_bench libhz11.so libhz11_span.so \
	      libhz11_stats.so libhz11_span_stats.so \
	      libhz11_top.so libhz11_span_top.so \
	      libhz11_tlsfast.so libhz11_span_tlsfast.so \
	      libhz11_nobytes.so libhz11_span_nobytes.so \
	      libhz11_soa.so libhz11_span_soa.so \
	      libhz11_span_transfer.so libhz11_span_transfer_thread_exit.so \
	      libhz11_span_transfer_thread_exit_cap.so \
	      libhz11_span_transfer_thread_exit_cap_source_diag.so \
	      libhz11_span_transfer_thread_exit_cap_batch32.so \
	      libhz11_span_transfer_thread_exit_cap_batch32_source_diag.so \
	      libhz11_span_transfer_thread_exit_cap_batch64.so \
	      libhz11_span_transfer_thread_exit_cap_batch128.so \
	      libhz11_span_transfer_thread_exit_cap_cap8192.so \
	      libhz11_span_transfer_thread_exit_cap_xferwide.so \
	      libhz11_span_transfer_diag.so \
	      libhz11_span_transfer_cap.so libhz11_span_return.so \
	      libhz11_span_source_diag.so libhz11_span_return_source_diag.so
