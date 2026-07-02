# HZ9 Segment Route Proofs L0

Detailed route/free-shape evidence for `HZ9SegmentLocalCache-L0`.

## Local Payload Probe

```text
command:
  ITERS=1000000 scripts/run_hz9_segment_local_payload_sweep.sh

legacy manual shape:
  for touch in 1 0; do
    for class_id in 0 1 2 3 4 5; do
      TOUCH=$touch CLASS_ID=$class_id ITERS=1000000 \
        h8_bench_hz9segmentlocalcache_local
    done
  done
```

## Route/Free Shape Summary

```text
direct known-slot local body:
  about 500-580M ops/s with real payload touch

public route + addr decode:
  about 84-124M ops/s

route returns class+slot:
  about 136-159M ops/s

active direct body:
  about 421-494M ops/s

active direct take + public route free:
  about 149-183M ops/s

active-first route then known-slot free:
  about 160-172M ops/s

active range-only check:
  about 185-202M ops/s

exact active no-fallback route:
  about 157-168M ops/s

sampled public-route proof:
  about 210-246M ops/s

slot-header route proof:
  about 206-219M ops/s

TLS last-token proof:
  about 204-216M ops/s

active direct take + known-slot free:
  about 220-241M ops/s

active direct take + active-class direct free:
  about 251-274M ops/s

one helper active take/free:
  about 245-275M ops/s

fused active take/free body:
  about 478-542M ops/s
```

## Interpretation

```text
route lookup is not the only tax:
  pair / pairfast / pairdirect remain far below active direct

free-side specialization helps:
  pairfast improves over pair

benchmark branch routing is not the blocker:
  pairdirect remains far below active direct

fused mutation body is the fast substrate:
  pairfused reaches about 1.14-1.34x active direct

behavior implication:
  local hit core must use a fused known-slot body
  public route validation must stay out of the immediate local reuse core
```

## Probe Meanings

```text
active_route=1:
  active direct take + route_table_slot free

active_route=2:
  active segment before route_table_slot free

active_route=3:
  active-first route then known-slot free

active_route=4:
  active range check then known-slot free

active_route=5:
  exact active route without table fallback

active_route=6:
  sampled route_table_slot proof every ROUTE_PROOF_INTERVAL

active_route=7:
  slot-header route then known-slot free

active_route=8:
  TLS last-token route then known-slot free

active_route=9:
  active direct take then known-slot free with no route

active_route=10:
  active direct take then active-class direct free

active_route=11:
  one helper active direct take/free

active_route=12:
  fused active direct take/free body
```

## Decision

```text
do not continue:
  simple public route sampling
  slot header route
  TLS last-token route
  split take/free helper tuning

continue:
  behavior substrate that keeps local hit on fused known-slot body
  public free route as a separate fail-closed boundary
```
