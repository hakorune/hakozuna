# SizePolicyV1Shadow-L1

Behavior change:

```text
none
```

Build:

```text
bench-release-audit
```

Command:

```bash
hakozuna-hz8/h8_bench_release_audit \
  --runs 3 \
  --threads 16 \
  --iters 100000 \
  --min-size 16 \
  --max-size 4096 \
  --remote-pct 90 \
  --interleaved 0
```

Raw log:

```text
bench_results/20260623T184720Z_size_policy_v1_shadow_phase_r3.log
```

## Result

```text
throughput median:
  3.50M ops/s

phase:
  alloc median 334.921ms
  remote median 11.260ms

remote live objects median:
  1,440,286

p2-v0 lower-bound spans median:
  60,314
```

Candidate lower-bound spans:

```text
upper1536:
  58,974

upper1p5:
  53,610
```

Remote-live rounded bytes:

```text
p2-v0:
  3,947,889,632

upper1536:
  3,855,197,152

upper1p5:
  3,485,462,496
```

Rounded-byte delta versus p2-v0:

```text
upper1536:
  -92,692,480 bytes
  -2.35%

upper1p5:
  -462,427,136 bytes
  -11.71%
```

Fragmentation over all allocations in the audit run:

```text
p2-v0 ratio:
  1.332783

upper1536 ratio:
  1.301518

upper1p5 ratio:
  1.176730
```

Safety:

```text
quiescent_pending bitmap_nonzero = 0
quiescent_pending repair = 0
slot_shadow mismatches = 0
```

## Interpretation

`upper1p5` remains the strongest observed size-policy candidate for reducing
small phase peak memory.  In this R3 shadow run it reduces remote-live rounded
bytes by about 11.7% and lower-bound spans by about 11.1%.

This does not justify changing the RC1 default.  It justifies a dedicated
paired A/B after the small/medium boundary policy is explicit.
