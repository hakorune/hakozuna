# PHASE S208-v3 Failure Root-Cause Work Order

Last updated: 2026-02-11  
Owner: external AI investigator  
Target: `hz3` medium path (`MT remote r90 16..32768`)

---

## 0) Goal

S208-v3 is active but not promotable:

- `RUNS=21` main: `23.166M -> 22.967M` (`-0.859%`)
- `RUNS=21` guard: `111.009M -> 110.319M` (`-0.621%`)

Task:

1. Explain why S208-v3 is still negative on main.
2. Propose and validate a fixable v4 shape (or confirm hard NO-GO).
3. Keep guard regression within `-1.0%`.

---

## 1) Fixed Inputs (Do not change first)

- Repo: `/mnt/workdisk/public_share/hakmem`
- Baseline SO: `/tmp/s208v3_promote_20260211_092927/base.so`
- Variant SO: `/tmp/s208v3_promote_20260211_092927/s208v3.so`
- Existing A/B logs:
  - main: `/tmp/s208v3_promote_20260211_092927/ab_main_r21/summary.txt`
  - guard: `/tmp/s208v3_promote_20260211_092927/ab_guard_r21/summary.txt`
- Existing S203 one-shot:
  - `/tmp/s208v3_counter_20260211_092405/run.out`
  - includes `[HZ3_S203_S208] reserve_attempts=26492 reserve_hits=26492 central_miss_after_reserve=0`

---

## 2) Mandatory Reproduction (exact order)

### 2.1 Strict preload verify

```bash
./scripts/verify_preload_strict.sh --so /tmp/s208v3_promote_20260211_092927/base.so --runs 1
./scripts/verify_preload_strict.sh --so /tmp/s208v3_promote_20260211_092927/s208v3.so --runs 1
```

### 2.2 Re-run AB (`RUNS=21`, interleaved)

Main:

```bash
hakozuna/scripts/ssot_ab_hz3.sh \
  --skip-build \
  --runs 21 --rss-runs 1 --skip-rss \
  --a-name base --b-name s208v3 \
  --a-built-so /tmp/s208v3_promote_20260211_092927/base.so \
  --b-built-so /tmp/s208v3_promote_20260211_092927/s208v3.so \
  --bench-bin ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  --bench-args '16 2000000 400 16 32768 90 65536' \
  --outdir /tmp/s208v3_investigate/ab_main_r21
```

Guard:

```bash
hakozuna/scripts/ssot_ab_hz3.sh \
  --skip-build \
  --runs 21 --rss-runs 1 --skip-rss \
  --a-name base --b-name s208v3 \
  --a-built-so /tmp/s208v3_promote_20260211_092927/base.so \
  --b-built-so /tmp/s208v3_promote_20260211_092927/s208v3.so \
  --bench-bin ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  --bench-args '16 2000000 400 16 2048 90 65536' \
  --outdir /tmp/s208v3_investigate/ab_guard_r21
```

### 2.3 Perf quick check (`main`, median of 2 runs)

Note: `ssot_ab_perf_gate.sh --skip-ab` currently fails due summary dependency.  
Use `--runs 1 --perf-runs 2`:

```bash
hakozuna/scripts/ssot_ab_perf_gate.sh \
  --a-so /tmp/s208v3_promote_20260211_092927/base.so \
  --b-so /tmp/s208v3_promote_20260211_092927/s208v3.so \
  --a-name base --b-name s208v3 \
  --runs 1 --perf-runs 2 \
  --bench-bin ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  --bench-args '16 2000000 400 16 32768 90 65536' \
  --outdir /tmp/s208v3_investigate/perf_main_r2
```

---

## 3) Root-Cause Questions (must answer with numbers)

1. Is reserve firing too often relative to benefit?
   - `reserve_attempts`, `reserve_hits`, main delta.
2. Does S208 reduce `from_segment` enough to pay for added overhead?
   - Compare `from_segment`, `from_central`, `alloc_slow_calls`.
3. Is added central snapshot/reserve bookkeeping dominating?
   - perf deltas on `cycles/op`, `instructions/op`, `branch-misses/op`.
4. Is problem concentrated in specific SC (`5/6/7`)?
   - Need SC-level `from_*` comparison.

---

## 4) Required Additional Instrumentation (minimal)

Use TLS counters only (no hot-path global atomics). Add temporary counters in S203 dump:

- `s208_reserve_objs_pushed` (sum of reserved objs pushed to central)
- `s208_reserve_paths_taken` (number of alloc_slow entries where reserve engaged)
- `s208_reserve_skipped_by_streak` (in-range miss but streak below threshold)

Optional (if low risk):

- `s208_reserve_sc[sc]` (per-sc reserved objs for sc5/6/7)

Do not add per-object logging.

---

## 5) v4 Candidate Sweep (short then deep)

### 5.1 Short screen (`RUNS=7`, main+guard only)

Test these 4 variants against base:

1. `STREAK_MIN=3`, others current
2. `STREAK_MIN=4`, others current
3. `STREAK_MIN=2`, `RESERVE_MAX_OBJS=2`
4. `STREAK_MIN=2`, `CENTRAL_FLOOR_OBJS=16`

### 5.2 Promote top-2 to `RUNS=21`

Promotion gate:

- main (`16..32768`) `>= 0%`
- guard (`16..2048`) `>= -1.0%`

If none pass, declare S208-v3 hard NO-GO and recommend archive.

---

## 6) Output Contract (files to deliver)

Write all outputs under one directory:

`/tmp/s208v3_deepcheck_YYYYmmdd_HHMMSS/`

Required files:

- `summary.tsv` (all AB medians and deltas)
- `perf_summary.md` (per-op key deltas)
- `counter_summary.md` (S203/S208 counters, SC focus)
- `root_cause.md` (why negative, prioritized)
- `recommendation.md` (promote / keep opt-in / archive)

Also include raw run directories for reproducibility.

---

## 7) Decision Rules

- `PROMOTE`: main non-negative, guard within gate, no obvious perf regression pattern.
- `OPT-IN`: mixed or unstable result with incomplete evidence.
- `ARCHIVE`: all tested v4 candidates fail gate or consistently negative on main.

---

## 8) Safety / Hygiene

- Do not use wrong `.so` paths. Always run strict preload verify first.
- Keep changes scoped to S208 boundary (`hz3_tcache_slowpath.inc`, S203 counters).
- Do not change unrelated defaults during this investigation.
- Fail-fast: if build or preload mismatch occurs, stop and report exact command/log.

