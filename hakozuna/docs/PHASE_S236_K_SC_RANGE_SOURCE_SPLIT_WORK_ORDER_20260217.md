# PHASE S236-K: SC-Range / Source-Split Work Order (Aggressive Lane)

Date: 2026-02-17  
Status: executed (NO-GO)

## Goal

- Confirm next `hz3` attack line after `S236-J` archive.
- Keep focus on `main` (`MT remote r90`, `16..32768`).
- Treat `large` as non-primary bottleneck for now (`S233` already default ON and positive).

## Locked Context

- `S236-A/B` is current aggressive baseline and remains default ON in scale lane.
- `main` is still inbox/central heavy (`from_inbox` + `from_central` dominant).
- Archived lines must not be replayed:
  - direct-to-central family (`S229/S230`)
  - central retry/hint family (`S236-E/F/G`)
  - mailbox multi-slot/second-chance/preload (`S236-D/I/J`)

## Why this phase

Recent NO-GO cases were mostly "good mechanism but wrong lane balance".  
Before adding new runtime state, first isolate where `S236-A/B` is actually profitable:

1. size-class scope (`sc` range)  
2. mini-refill source mix (`inbox` vs `central-fast`)

This phase is **no-code first** and uses existing knobs only.

---

## Stage A: Existing Knob Sweep (No Code Change)

### Build matrix

Base (`current default`):
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S203_COUNTERS=1'
```

Variants:

1. `v1_sc6_7`
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S236_SC_MIN=6 -DHZ3_S236_SC_MAX=7
```

2. `v2_sc5_6`
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S236_SC_MIN=5 -DHZ3_S236_SC_MAX=6
```

3. `v3_sc6_only`
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S236_SC_MIN=6 -DHZ3_S236_SC_MAX=6
```

4. `v4_inbox_only`
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_TRY_CENTRAL_FAST=0
```

5. `v5_central_only`
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S236_MINIREFILL_TRY_INBOX=0
```

### Bench protocol

- Interleaved A/B, pinned `0-15`.
- `RUNS=10` screen.

Lanes:
- `main`: `16 2000000 400 16 32768 90 65536`
- `guard`: `16 2000000 400 16 2048 90 65536`
- `larson`: `4KB..32KB`

### Screen gates (vs base)

- `main >= +3.0%`
- `guard >= -3.0%`
- `larson >= -3.0%`
- hard kill: `alloc_slow_calls <= +5%`

### Required counters (S203)

- `alloc_slow_calls`
- `alloc_slow_from_inbox`
- `alloc_slow_from_central`
- `s236_mini_calls`
- `s236_mini_hit_inbox`
- `s236_mini_hit_central`

Deliverables:
- `/tmp/s236k_sweep_*/raw.tsv`
- `/tmp/s236k_sweep_*/summary.tsv`
- `/tmp/s236k_sweep_*/REPORT.md`

---

## Stage B: Promotion / Stop Rule

### If one variant passes all screen gates

- Run replay (`RUNS=21`) only for the winning variant.
- Replay gates:
  - `main >= +5.0%`
  - `guard >= -4.0%`
  - `larson >= -4.0%`
  - crash/abort/alloc-failed = 0

If replay passes:
- promote winning config as new aggressive baseline (`S236-K tuned`).

### If all variants fail

- Do not add new micro-boxes on S236 line in this phase.
- Mark `S236-K knob split` as NO-GO.
- Move to next major design step outside S236 micro family.

---

## Notes

- This phase intentionally avoids new TLS arrays / hot-path state.
- It is a lane-shape isolation pass to avoid repeating high-variance NO-GO patterns.

---

## Execution Result (2026-02-17, RUNS=10)

Run artifacts:
- `/tmp/s236k_sweep_safe_run10b_20260217_112310/summary.tsv`
- `/tmp/s236k_sweep_safe_run10b_20260217_112310/raw.tsv`
- `/tmp/s236k_sweep_safe_run10b_20260217_112310/counters.tsv`
- `/tmp/s236k_sweep_safe_run10b_20260217_112310/build_failures.tsv`

Summary (median, vs per-pair base):

| config | main | guard | larson_proxy | verdict |
|---|---:|---:|---:|---|
| v1_sc6_7 | -7.29% | -4.29% | +0.65% | FAIL |
| v2_sc5_6 | -11.39% | +9.62% | +0.22% | FAIL |
| v3_sc6_only | -21.14% | +0.89% | +0.77% | FAIL |
| v4_inbox_only | -19.41% | +1.56% | +1.69% | FAIL |
| v5_central_only | build fail | build fail | build fail | SKIP |

Decision:
- **S236-K NO-GO**. No variant satisfies `main >= +3.0%`.
- Keep current S236-A/B default shape (`SC 5..7`, inbox+central-fast both ON).

Additional notes:
- `v5_central_only` does not compile under current tree with `-Werror` because `HZ3_S236_MINIREFILL_TRY_INBOX=0` triggers unused variable (`hz3_tcache_slowpath.inc`).
- `alloc_slow_calls` median increased in all tested variants vs base (main lane), consistent with throughput regression.
