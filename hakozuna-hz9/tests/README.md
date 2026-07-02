# HZ9 Tests

HZ9 has its own standalone test binaries.

```text
make -C hakozuna-hz9 smoke
./hakozuna-hz9/h8_smoke
```

Current standalone smoke set:

```text
make -C hakozuna-hz9 smoke
  inherited allocator boundary smoke

make -C hakozuna-hz9 preload-smoke
  local preload library sanity check

make -C hakozuna-hz9 smoke-hz9segmentlocalcache
  HZ9 segment local cache put/take/free, remote contamination, drain, and
  release-all scaffold smoke

make -C hakozuna-hz9 smoke-hz9slabroute
  HZ9 slab route VALID / INVALID / MISS classification

make -C hakozuna-hz9 smoke-hz9slabpage
  HZ9 slab page local, remote, owner-exit, and final-free behavior

make -C hakozuna-hz9 smoke-hz9ownerpagepool-route
  HZ9 owner-page exact route / INVALID / MISS and state transition smoke

make -C hakozuna-hz9 smoke-hz9ownerpagepool-api
  HZ9 owner-page API local pop/free, double-free rejection, interior INVALID,
  remote pending repeat rejection, and detached final-free release
```

Current standalone probe benches:

```text
make -C hakozuna-hz9 bench-hz9segmentlocalcache-api
  SegmentLocalCache API shape sweep

make -C hakozuna-hz9 bench-hz9segmentlocalcache-local
  SegmentLocalCache direct known-slot cycle on a real one-run payload
  set ROUTE_FREE=1 to include table route + addr->slot free validation
  set ROUTE_FREE=2 to return class+slot from table route before free
  set ACTIVE_CYCLE=1 to use active-segment direct known-slot cycling

ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_local_payload_sweep.sh
  SegmentLocalCache real-payload sweep for direct / active / route2 modes
```

Active-run magazine evidence tests are still available as historical L0
coverage:

```text
make -C hakozuna-hz9 smoke-hz9mediumlocalmag
./hakozuna-hz9/h8_smoke_hz9mediumlocalmag
```

Covered by the magazine smoke:

```text
LOCAL_MAG double free rejection
remote publish rejection for LOCAL_MAG
owner-exit magazine flush
owner-exit empty-magazine hard drain
48K / 64K active-run magazine push/pop
state_mismatch == 0
```

Must remain covered by inherited boundary gates:

```text
invalid-pointer fail-closed behavior
remote duplicate-claim behavior
owner-exit hard drain
cache residue after owner exit
```
