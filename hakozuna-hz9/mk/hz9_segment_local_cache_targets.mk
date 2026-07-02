$(ROOT)/h8_hz9_segment_local_cache_smoke: $(HZ9_SEGMENT_LOCAL_CACHE_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_SEGMENT_LOCAL_CACHE_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_LOCAL_CACHE_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-hz9segmentlocalcache: $(ROOT)/h8_hz9_segment_local_cache_smoke
	$(ROOT)/h8_hz9_segment_local_cache_smoke

$(ROOT)/h8_hz9_segment_entry_smoke: $(HZ9_SEGMENT_ENTRY_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_SEGMENT_ENTRY_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_ENTRY_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-hz9segmententry: $(ROOT)/h8_hz9_segment_entry_smoke
	$(ROOT)/h8_hz9_segment_entry_smoke

$(ROOT)/h8_bench_hz9segmententry: $(HZ9_SEGMENT_ENTRY_BENCH_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_SEGMENT_ENTRY_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_ENTRY_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-hz9segmententry: $(ROOT)/h8_bench_hz9segmententry
	$(ROOT)/h8_bench_hz9segmententry

$(ROOT)/h8_bench_hz9segmentlocalcache_api: $(HZ9_SEGMENT_LOCAL_CACHE_BENCH_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_SEGMENT_LOCAL_CACHE_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_LOCAL_CACHE_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-hz9segmentlocalcache-api: $(ROOT)/h8_bench_hz9segmentlocalcache_api
	$(ROOT)/h8_bench_hz9segmentlocalcache_api

$(ROOT)/h8_bench_hz9segmentlocalcache_local: $(HZ9_SEGMENT_LOCAL_CACHE_LOCAL_BENCH_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_SEGMENT_LOCAL_CACHE_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_LOCAL_CACHE_LOCAL_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-hz9segmentlocalcache-local: $(ROOT)/h8_bench_hz9segmentlocalcache_local
	$(ROOT)/h8_bench_hz9segmentlocalcache_local
