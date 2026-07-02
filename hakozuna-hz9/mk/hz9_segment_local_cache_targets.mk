$(ROOT)/h8_hz9_segment_local_cache_smoke: $(HZ9_SEGMENT_LOCAL_CACHE_SMOKE_SRC) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(HZ9_SEGMENT_LOCAL_CACHE_CFLAGS) $(INC) -o $@ $(HZ9_SEGMENT_LOCAL_CACHE_SMOKE_SRC) $(LDFLAGS) $(LDLIBS)

smoke-hz9segmentlocalcache: $(ROOT)/h8_hz9_segment_local_cache_smoke
	$(ROOT)/h8_hz9_segment_local_cache_smoke
