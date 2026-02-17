# PHASE S236-Q: Inbox Drain Empty Backoff Work Order

Date: 2026-02-18
Status: **NO-GO**

## Context

S236-P (producer-side batching) was NO-GO due to:
- Main: +1.03% (gate +2.5% failed)
- Guard: -8.52% (gate -3.0% failed)

S236-Q takes a different approach: consumer-side optimization by skipping inbox drain calls when the inbox is likely empty.

## Stage A: Observation Results

Run artifacts: `/tmp/s236q_observe_20260218_010822/`

**Main lane (16..32768 r90):**
- empty_ratio: **38.8%** >= 35% ✓
- from_inbox: 64.99% >= 20% ✓
- from_central: 29.31% >= 20% ✓

**Guard lane (16..2048 r90):**
- empty_ratio: 100% >= 20% ✓

**Result:** All Stage B conditions met, proceeded to implementation.

## Stage B: Implementation

### Knobs

```c
HZ3_S236Q_INBOX_DRAIN_EMPTY_BACKOFF (default 0)
HZ3_S236Q_EMPTY_STREAK_MIN (default 3)
HZ3_S236Q_BACKOFF_WINDOW (default 8)
HZ3_S236Q_BACKOFF_PROBE_EVERY (default 4)
```

### Algorithm

- Per-SC TLS: `empty_streak[sc]`, `backoff_left[sc]`, `probe_tick[sc]`
- Skip inbox drain when `backoff_left > 0` (except on probe ticks)
- Set backoff when `empty_streak >= STREAK_MIN`
- Reset on inbox hit

### Files Changed

- `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc`
- `hakozuna/hz3/src/hz3_tcache.c` (S203 counters)
- `hakozuna/hz3/src/hz3_tcache_slowpath.inc` (implementation)

## A/B Test Results (RUNS=10, interleaved)

Run artifacts: `/tmp/s236q_ab_20260218_011723/`

| Lane | BASE (ops/s) | VAR (ops/s) | Change | Gate | Pass |
|------|--------------|-------------|--------|------|------|
| main | 34.4M | 34.3M | **-0.32%** | >= +2.5% | **FAIL** |
| guard | 91.2M | 86.3M | **-5.41%** | >= -3.0% | **FAIL** |
| larson | 145.9M | 148.8M | +2.01% | >= -3.0% | PASS |

### Hard-kill Conditions

- `alloc_slow_calls`: BASE=988,257 VAR=971,347 (-1.71%) ✓
- `from_central`: BASE=313,753 VAR=345,504 (**+10.11%**) ✗

### Mechanism Validation

- `backoff_skip=185,566` (mechanism active)
- `backoff_probe=61,596`
- `backoff_set=61,908`
- `backoff_hit_recover=99,831`

## Decision: S236-Q NO-GO

**Reasons:**
1. Main gate failed (-0.32% vs required +2.5%)
2. Guard gate failed (-5.41% vs required -3.0%)
3. Hard-kill: `from_central` +10.11% > +10% threshold

**Root Cause:**
When inbox drain is skipped, the fallback to central pop increases:
- `from_central` increased by 10.11%
- This adds lock contention and reduces throughput in guard lane

The backoff mechanism correctly identifies empty inbox patterns, but the cost of additional central pops outweighs the savings from skipped inbox drain calls. The existing inbox drain already has low overhead (batch return), so skipping it doesn't provide sufficient benefit.

**Conclusion:** Consumer-side inbox drain backoff is not beneficial for the tested workload patterns.

## Archive

- Research archive: `hakozuna/hz3/archive/research/s236q_inbox_drain_empty_backoff/`
- Source-path changes: Revert after NO-GO confirmation
