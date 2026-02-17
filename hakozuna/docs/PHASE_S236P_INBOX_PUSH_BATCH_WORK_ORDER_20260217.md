# PHASE S236-P: Inbox Push Batching Work Order

Date: 2026-02-17
Status: **NO-GO**

## Context

Stage A observation (main lane r90, RUNS=7) indicates:

- `alloc_slow_from_inbox_ratio`: 65.77%
- `alloc_slow_from_central_ratio`: 29.85%
- `inbox_push_objs_per_call`: 1.000
- `inbox_drain_objs_per_call`: 7.884
- `inbox_drain_objs_per_nonempty`: 12.761

Interpretation:

- Inbox drain already has non-trivial batching.
- Producer side is mostly single-object push to inbox (`push_list` used as `n=1`).
- Next win should target **producer batching**, not additional drain complexity.

## Goal

Reduce `hz3_inbox_push_list` call frequency in main lane (r90) by batching remote pushes per `(dst, sc)`.

Primary objective:

- main lane (16..32768 r90) >= +2.5%

Safety objectives:

- guard >= -3.0%
- larson_proxy >= -3.0%

## Box

### S236-P: Inbox Producer Batch Box (opt-in)

- Add a tiny TLS pending slot array for medium remote pushes:
  - key: `dst + sc`
  - value: `(head, tail, count)`
- On remote free path:
  - append object to pending slot instead of immediate `hz3_inbox_push_list(...,1)`
  - flush when `count >= HZ3_S236P_PUSH_BATCH_N`
- Flush points:
  - before thread exit
  - before slowpath paths that require visibility
  - on slot overwrite/conflict

Constraints:

- no change in free dispatch boundary
- default OFF (`HZ3_S236P_INBOX_PUSH_BATCH=0`)
- fail-fast checks retained (`watch/debug` paths unchanged)

## Knobs

In `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`:

```c
#ifndef HZ3_S236P_INBOX_PUSH_BATCH
#define HZ3_S236P_INBOX_PUSH_BATCH 0
#endif
#ifndef HZ3_S236P_PUSH_BATCH_N
#define HZ3_S236P_PUSH_BATCH_N 4
#endif
#ifndef HZ3_S236P_MAX_PENDING_KEYS
#define HZ3_S236P_MAX_PENDING_KEYS 8
#endif
```

## Counters (S203)

Add:

- `s236p_push_batch_flush_calls`
- `s236p_push_batch_flush_objs`
- `s236p_push_batch_conflict_flush`
- `s236p_push_batch_tls_overflow`

Expected direction:

- `inbox_push_calls` down
- `inbox_push_objs_per_call` up

## A/B Protocol

Base:

- `-DHZ3_S203_COUNTERS=1`

Var:

- `-DHZ3_S203_COUNTERS=1 -DHZ3_S236P_INBOX_PUSH_BATCH=1`

Lanes (RUNS=10, interleaved, pinned 0-15):

- main: `16 2000000 400 16 32768 90 65536`
- guard: `16 2000000 400 16 2048 90 65536`
- larson_proxy: `4 2500000 400 4096 32768 0 65536`

Screen gates:

- main >= +2.5%
- guard >= -3.0%
- larson_proxy >= -3.0%

Hard-kill:

- `alloc_slow_calls` > +5%

If screen passes:

- replay RUNS=21

## Stage A Script

Observation helper added:

- `hakozuna/hz3/scripts/run_s236p_inbox_observe.sh`

It builds with `HZ3_S203_COUNTERS=1`, runs main-r90, and records:

- `summary.tsv` (ops median, inbox/drain efficiency)
- `perf_focus.tsv` (bench_thread, inbox_drain, inbox_push_list lines)

---

## Execution Result (2026-02-18, RUNS=10, corrected)

Run artifacts: `/tmp/s236p_ab_20260218_014748/`

**Note:** First run used wrong .so file (symlink to old baseline). This is the corrected run using actual build output.

### Benchmark Results

| Lane | BASE (ops/s) | VAR (ops/s) | Change | Gate | Pass |
|------|--------------|-------------|--------|------|------|
| main | 32.1M | 32.4M | +1.03% | >= +2.5% | **FAIL** |
| guard | 93.4M | 85.4M | **-8.52%** | >= -3.0% | **FAIL** |
| larson | 146.5M | 148.2M | +1.21% | >= -3.0% | PASS |

### Mechanism Validation

S236-P mechanism confirmed active:
- `push_batch_flush_calls` = 4,624,036
- `push_batch_flush_objs` = 12,813,454
- `inbox_push_calls`: BASE=12,864,618 → VAR=4,624,036 (**64.0% reduction**)
- `inbox_push_objs_per_call`: 1.0 → ~2.77

### Decision: S236-P NO-GO

**Reason:** Two gates failed (main +1.03% < +2.5%, guard -8.52% < -3.0%)

**Analysis:**
- Mechanism works as intended (64% reduction in inbox_push_calls)
- But the batching overhead (TLS slot lookup, object linking, flush checks) outweighs the benefit
- Guard lane severely impacted (-8.52%) - likely due to different allocation pattern
- Main lane shows modest improvement (+1.03%) but below target
- Larson lane unaffected (+1.21%)

**Root cause:** Producer-side batching at `hz3_medium_dispatch_push` adds fixed per-object cost (slot search, linking) that is not amortized enough by the reduced inbox_push_list calls. The S199/S236C mechanisms already provide batching at different levels.

**Conclusion:** Producer-side batching at `hz3_medium_dispatch_push` level is NO-GO. The approach adds fixed cost without sufficient benefit for the workload patterns tested.

## Archive

- Research archive:
  - `hakozuna/hz3/archive/research/s236p_inbox_push_batch/README.md`
- Source-path changes were reverted after NO-GO confirmation.
