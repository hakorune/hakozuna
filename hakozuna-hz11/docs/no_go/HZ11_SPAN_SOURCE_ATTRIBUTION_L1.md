# HZ11SpanSourceAttribution-L1

Status: GO for attribution; NO promotion or optimization claim.

This box asks where the remaining macro RSS and span-return wall-time failures
come from after `HZ11CentralFreeListSpanReturn-L1`. It adds opt-in source
counters only. The default HZ11 lanes and the fixed-local hit path are not
changed.

## Boundary

```text
Opt-in flags:
  HZ11_SPAN_SOURCE_DIAG=1
  HZ11_DUMP_SPAN_SOURCE=1

Diagnostic siblings:
  libhz11_span_source_diag.so
  libhz11_span_return_source_diag.so

Runner:
  scripts/run_hz11_span_source_attribution.sh

Output:
  bench_results/hz11_span_source_attr_20260707T231924Z/summary.md
```

The diagnostic records class-level span source counts:

```text
span_create
span_reuse
current_span_exhaust
current_span_replace
transfer_refill_hit / transfer_refill_miss
central_refill_hit / central_refill_miss
arena_carve
live_current_span
created_high_water
sweep_count / sweep_scanned
meta_lock
```

## Median Samples

RUNS=3. RSS values are KiB.

| Workload | Allocator | Status | Wall sec | Max RSS | Current RSS |
|---|---|---:|---:|---:|---:|
| larson | tcmalloc | OK:3 | 4.141 | 278784 | 278784 |
| larson | hz11-span-transfer | OK:3 | 4.174 | 653952 | 653952 |
| larson | hz11-span-return | OK:3 | 4.177 | 655232 | 655232 |
| python_alloc | tcmalloc | OK:3 | 0.972 | 104064 | 104064 |
| python_alloc | hz11-span-transfer | FAIL:3 | NA | NA | NA |
| python_alloc | hz11-span-return | OK:3 | 0.909 | 118620 | 118532 |
| mstress | tcmalloc | OK:3 | 0.190 | 212224 | 211840 |
| mstress | hz11-span-transfer | FAIL:3 | NA | NA | NA |
| mstress | hz11-span-return | OK:3 | 0.241 | 261420 | 213632 |
| sh6bench | tcmalloc | OK:3 | 0.353 | 258816 | 249856 |
| sh6bench | hz11-span-transfer | OK:3 | 4.545 | 352256 | 352256 |
| sh6bench | hz11-span-return | OK:3 | 18.812 | 352512 | 352512 |

## Attribution

### Larson RSS

The larson RSS source is not the central stack. It is dominated by fresh
current-span allocation from the arena in classes 0-3.

| Allocator | class | span_create | arena_carve | span_reuse | central_bypass_rate | live_current_span |
|---|---:|---:|---:|---:|---:|---:|
| hz11-span-transfer | 2 | 50589 | 50589 | 0 | 1.000 | 50589 |
| hz11-span-transfer | 3 | 50438 | 50438 | 0 | 1.000 | 50438 |
| hz11-span-transfer | 1 | 48798 | 48798 | 0 | 1.000 | 48798 |
| hz11-span-transfer | 0 | 46780 | 46780 | 0 | 1.000 | 46780 |
| hz11-span-return | 2 | 50585 | 50585 | 0 | 1.000 | 50585 |
| hz11-span-return | 3 | 50441 | 50441 | 0 | 1.000 | 50441 |
| hz11-span-return | 1 | 48800 | 48800 | 0 | 1.000 | 48800 |
| hz11-span-return | 0 | 46779 | 46779 | 0 | 1.000 | 46779 |

The important shape is:

```text
span_create == arena_carve
span_reuse == 0
central_bypass_rate == 1.000
live_current_span ~= span_create
```

That means larson's 2.35x RSS is caused by per-thread current-span / span-source
policy. CentralFreeList span return cannot fix it because those spans do not
become central-only full spans.

### Python Alloc / Mstress

`hz11-span-return` remains useful as a correctness attribution lane for the
former abort rows:

```text
python_alloc: OK 3/3, max RSS 1.14x tcmalloc
mstress:      OK 3/3, max RSS 1.23x tcmalloc
```

`python_alloc` mostly reuses central/transfer paths after the cap/return lane:
its top class has central_bypass_rate 0.028. `mstress` still has fresh carve in
large classes, with some visible reuse in class 5.

### Sh6bench Wall Regression

The sh6bench span-return wall regression is not from the full-span sweep:

```text
sweep_scanned == 0
sweep_count == 0
```

It is instead attributable to metadata-lock traffic in span-return accounting:

| class | meta_lock |
|---:|---:|
| 0 | 762497136 |
| 1 | 137392432 |
| 2 | 109062736 |
| 3 | 58087408 |
| 5 | 5624544 |

The next span-return box should not tune sweep shape first. It should remove or
batch the per-object metadata-lock accounting if span-return is retried for
this workload shape.

## Decision

GO for attribution. The root causes are specific enough to split the next work:

```text
larson:
  current-span / thread-churn span pooling is the primary RSS lever.

sh6bench:
  span-return metadata lock accounting is the wall-time lever.
  sweep/O(n) is ruled out by this measurement.
```

Do not promote `libhz11_span_return.so`. Keep `libhz11_span_transfer.so` as the
remote/mixed microbench speed lane only until the current-span source policy is
changed and remeasured.
