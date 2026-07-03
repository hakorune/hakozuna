$(ROOT)/h8_hz9_segment_local_cache_smoke: $(HZ9_SEGMENT_LOCAL_CACHE_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_SEGMENT_LOCAL_CACHE_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_LOCAL_CACHE_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-hz9segmentlocalcache: $(ROOT)/h8_hz9_segment_local_cache_smoke
	$(ROOT)/h8_hz9_segment_local_cache_smoke

$(ROOT)/h8_hz9_segment_entry_smoke: $(HZ9_SEGMENT_ENTRY_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_SEGMENT_ENTRY_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_ENTRY_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-hz9segmententry: $(ROOT)/h8_hz9_segment_entry_smoke
	$(ROOT)/h8_hz9_segment_entry_smoke

$(ROOT)/h8_hz9_local_slab_route_boundary_smoke: $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_CFLAGS) $(INC) -o $@ $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-hz9localslabrouteboundary: $(ROOT)/h8_hz9_local_slab_route_boundary_smoke
	$(ROOT)/h8_hz9_local_slab_route_boundary_smoke

$(ROOT)/h8_bench_hz9localslabrouteboundary: $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_BENCH_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_CFLAGS) $(INC) -o $@ $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-hz9localslabrouteboundary: $(ROOT)/h8_bench_hz9localslabrouteboundary
	$(ROOT)/h8_bench_hz9localslabrouteboundary

$(ROOT)/h8_bench_hz9publicapi_baseline: $(HZ9_PUBLIC_API_BASELINE_BENCH_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_LOCAL_SLAB_ROUTE_BOUNDARY_CFLAGS) $(INC) -o $@ $(HZ9_PUBLIC_API_BASELINE_BENCH_SRC) $(LDFLAGS) $(LDLIBS)

bench-hz9publicapi-baseline: $(ROOT)/h8_bench_hz9publicapi_baseline
	$(ROOT)/h8_bench_hz9publicapi_baseline

$(ROOT)/h8_bench_release_hz9localslabpublicentry: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_LOCAL_SLAB_PUBLIC_ENTRY_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

bench-release-hz9localslabpublicentry: $(ROOT)/h8_bench_release_hz9localslabpublicentry

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
