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
  set ACTIVE_ROUTE=1 to use active direct take + route_table_slot free
  set ACTIVE_ROUTE=2 to try active segment before route_table_slot free
  set ACTIVE_ROUTE=3 to try active segment route then known-slot free
  set ACTIVE_ROUTE=4 to try active range check then known-slot free
  set ACTIVE_ROUTE=5 to try active exact route without table fallback
  set ACTIVE_ROUTE=6 to sample route_table_slot proof every ROUTE_PROOF_INTERVAL
  set ACTIVE_ROUTE=7 to try slot-header route then known-slot free
  set ACTIVE_ROUTE=8 to try TLS last-token route then known-slot free
  set ACTIVE_ROUTE=9 to try active direct take then known-slot free without route
  set ACTIVE_ROUTE=10 to try active direct take then active-class direct free
  set ACTIVE_ROUTE=11 to try one helper active direct take/free
  set ACTIVE_ROUTE=12 to try fused active direct take/free body

ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_local_payload_sweep.sh
  SegmentLocalCache real-payload sweep for direct / active / active_route /
  active_fast / active_route_probe / active_range_probe /
  active_exact_probe / active_sample8 / active_sample64 /
  active_header_probe / active_token_probe / active_pair_probe /
  active_pair_fast_free / active_pair_direct / active_pair_fused / route2 modes

ITERS=1000000 hakozuna-hz9/scripts/run_hz9_segment_route_proof_gate.sh
  Focused SegmentLocalCache direct/public/sample route-proof gate

make -C hakozuna-hz9 bench-hz9segmententry
  SegmentEntry global-routeable page scaffold
  set MODE=route, MODE=fused, MODE=fast, MODE=page, MODE=handle, or MODE=tls
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
