$(ROOT)/h8_hz9_static_local_page_smoke: $(HZ9_STATIC_LOCAL_PAGE_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_STATIC_LOCAL_PAGE_SCAFFOLD_CFLAGS) $(INC) -o $@ $(HZ9_STATIC_LOCAL_PAGE_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

$(ROOT)/h8_bench_release_hz9staticlocalpage_scaffold: $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(HZ9_STATIC_LOCAL_PAGE_SCAFFOLD_CFLAGS) $(MEDIUM_COLLECT_CFLAGS) $(INC) -o $@ $(SRC) $(ROOT)/bench/h8_bench.c $(BENCH_SUPPORT_SRC) $(BENCH_REPORT_SRC) $(BENCH_WORKERS_SRC) $(LDFLAGS) $(LDLIBS)

bench-release-hz9staticlocalpage-scaffold: $(ROOT)/h8_bench_release_hz9staticlocalpage_scaffold
