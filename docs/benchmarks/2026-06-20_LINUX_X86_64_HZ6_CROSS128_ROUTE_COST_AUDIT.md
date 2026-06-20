# Linux x86_64 HZ6 Cross128 Route Cost Audit, 2026-06-20

Diagnostic-only observation for the `cross128_r90` MT remote row:

```text
threads=16 iters=120000 ws=100 size=128..128 remote_pct=90 ring_slots=65536
```

## Command

```bash
./hakozuna-hz6/linux/run_hz6_preload_owner_inbox_tax_ab.sh \
  --runs 3 \
  --variants p0_off,p0_transfer_class_presence_min192,p0_transfer_presence_small_class \
  --rows cross128_r90 \
  --diagnostic
```

## Raw Results

- `hakozuna-hz6/private/raw-results/linux/hz6_cross128_route_cost_audit_20260620_163848`

## Median Diagnostic Counters

| counter | p0_off | presence_min192 | presence+small_class |
| --- | ---: | ---: | ---: |
| ops/s | 0.387M | 0.392M | 0.387M |
| returned_backpressure | 125,512 | 125,733 | 125,029 |
| transfer_pop | 514,642 | 480,924 | 375,324 |
| frontcache_reuse_hit | 201,798 | 198,082 | 186,308 |
| transfer_reserve_full | 760,803 | 727,732 | 624,397 |
| origin_transfer_success | 510,635 | 476,421 | 374,364 |
| origin_transfer_full | 125,512 | 125,733 | 125,029 |
| origin pop loop empty | 736,821 | 849,444 | 1,200,791 |
| origin pop loop hit | 514,642 | 480,924 | 375,324 |
| pop empty transfer count total | 44.04M | 47.99M | 22.33M |
| class-presence empty scan total | 0 | 58.92M | 33.06M |

## Read

- `cross128_r90` is dominated by tiny-object remote-free transfer pressure, not
  source allocation.  `toy_source_alloc` is only about 1K.
- Small-class sharding reduces `transfer_pop`, origin-transfer success, and
  class-presence empty scan work, which matches its production cross128 win
  inside HZ6.
- The row still has about 125K returned backpressure events and a large
  transfer-full tail in all HZ6 variants.
- The next clean box should target Toy/cross128 transfer cache shape or a
  same-size remote-free fast path.  Broad profile mixing remains no-go.
