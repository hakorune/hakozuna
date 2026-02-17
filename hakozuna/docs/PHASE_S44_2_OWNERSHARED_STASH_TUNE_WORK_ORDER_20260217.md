# PHASE S44-2: OwnerSharedStash Tune Work Order (No-Code)

Date: 2026-02-17
Status: executed (NO-GO)

## Goal

- Tune S44 (OwnerSharedStash) parameters for main lane improvement.
- S44 is SOFT GO (+8.8% on remote-heavy small), push to +10%+.
- No code change, knob sweep only.

## Context

- S236-K/L both NO-GO. S236 micro-tuning line frozen.
- S44 infrastructure is already in place and showing positive results.
- Current bottleneck: `alloc_slow_calls` ~600K-620K in main lane.

## Build Matrix

Base:
```sh
-DHZ3_S203_COUNTERS=1 -DHZ3_S44_4_STATS=1
```

Variants:
```sh
v1_d64       = BASE -DHZ3_S44_DRAIN_LIMIT=64
v2_d128      = BASE -DHZ3_S44_DRAIN_LIMIT=128
v3_bd16      = BASE -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=16
v4_bd32      = BASE -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=32
v5_d64_bd16  = BASE -DHZ3_S44_DRAIN_LIMIT=64 -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=16
v6_d128_bd16 = BASE -DHZ3_S44_DRAIN_LIMIT=128 -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=16
```

## Lanes and Runs

- Interleaved A/B
- `RUNS=10` (screen)
- pinned cores: `0-15`

Lanes:
- `main`: `16 2000000 400 16 32768 90 65536`
- `guard`: `16 2000000 400 16 2048 90 65536`
- `larson_proxy`: `4 2500000 400 4096 32768 0 65536`

## Gates

- `main >= +3.0%`
- `guard >= -3.0%`
- `larson_proxy >= -3.0%`
- hard kill: `alloc_slow_calls` median increase > +5% vs base (check `*.err` for `[HZ3_S203]`)

## Required Counters (S203 + S44)

- `alloc_slow_calls`
- `alloc_slow_from_inbox`
- `alloc_slow_from_central`
- `s44_drain_calls` (if available)
- `s44_drain_success` (if available)

## Decision

- If 1-2 variants pass all screen gates: run replay (`RUNS=21`) on winners only.
- Replay gates:
  - `main >= +5.0%`
  - `guard >= -4.0%`
  - `larson_proxy >= -4.0%`
  - crash/abort/calloc-failed = 0

- If all variants fail:
  - mark **S44-2 NO-GO**
  - freeze S44 tuning line
  - move to next box (S42 Transfer Cache or S236-M)

---

## Execution Result (2026-02-17, RUNS=10)

Run artifacts:
- `/tmp/s44_2_sweep_safe_20260217_124538/summary.tsv`
- `/tmp/s44_2_sweep_safe_20260217_124538/raw.tsv`
- `/tmp/s44_2_sweep_safe_20260217_124538/counters.tsv`

Summary (median, vs per-pair base):

| config | main | guard | larson_proxy | verdict |
|---|---:|---:|---:|---|
| v1_d64 | -2.64% | +1.87% | +1.79% | FAIL |
| v2_d128 | -2.06% | +2.36% | -0.33% | FAIL |
| v3_bd16 | -2.35% | -1.06% | -2.09% | FAIL |
| v4_bd32 | +0.75% | -1.82% | +0.01% | FAIL |
| v5_d64_bd16 | -2.14% | -5.03% | +1.03% | FAIL |
| v6_d128_bd16 | -1.28% | +6.09% | +1.18% | FAIL |

Decision:
- **S44-2 NO-GO**. No variant satisfies `main >= +3.0%` gate.
- Most variants are negative on main lane; best is `v4_bd32` at `+0.75%`, still below gate.

Root cause analysis:
- counter shape is mixed (some variants increase `from_central`, some decrease), but
  throughput gain on `main` is consistently insufficient.
- current S44 default appears close to local optimum for this lane set.
- parameter-only tuning does not provide a stable lane-wide win.

Next steps:
- Freeze S44 tuning line.
- Move to next optimization box (S42 Transfer Cache or different approach).

## Notes

- `HZ3_S44_DRAIN_EVERY_N` does not exist in current code - do not use.
- Focus on `DRAIN_LIMIT` and `BOUNDED_DRAIN` combinations.
- Bounded drain adds extra objects when draining under pressure.

## Run Script

```bash
cd /mnt/workdisk/public_share/hakmem
set -euo pipefail

OUT="/tmp/s44_2_sweep_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUT"/{bin,logs}
echo "OUT=$OUT"

BENCH="./hakozuna/out/bench_random_mixed_mt_remote_malloc"
RUNS="${RUNS:-10}"
BASE_DEFS="-DHZ3_S203_COUNTERS=1 -DHZ3_S44_4_STATS=1"

# lanes
LANE_main="16 2000000 400 16 32768 90 65536"
LANE_guard="16 2000000 400 16 2048 90 65536"
LANE_larson_proxy="4 2500000 400 4096 32768 0 65536"

# variants (S44 knobs only)
declare -A VAR
VAR[base]="$BASE_DEFS"
VAR[v1_d64]="$BASE_DEFS -DHZ3_S44_DRAIN_LIMIT=64"
VAR[v2_d128]="$BASE_DEFS -DHZ3_S44_DRAIN_LIMIT=128"
VAR[v3_bd16]="$BASE_DEFS -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=16"
VAR[v4_bd32]="$BASE_DEFS -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=32"
VAR[v5_d64_bd16]="$BASE_DEFS -DHZ3_S44_DRAIN_LIMIT=64 -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=16"
VAR[v6_d128_bd16]="$BASE_DEFS -DHZ3_S44_DRAIN_LIMIT=128 -DHZ3_S44_BOUNDED_DRAIN=1 -DHZ3_S44_BOUNDED_EXTRA=16"

build_so () {
  local name="$1"
  make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA="${VAR[$name]}" >/dev/null
  cp -f libhakozuna_hz3_scale.so "$OUT/bin/${name}.so"
}

for v in base v1_d64 v2_d128 v3_bd16 v4_bd32 v5_d64_bd16 v6_d128_bd16; do
  build_so "$v"
done

run_once () {
  local pair="$1" side="$2" lane="$3" run_id="$4" args="$5"
  local so="$OUT/bin/${side}.so"
  local out err ops
  out="$(taskset -c 0-15 env LD_PRELOAD="$so" $BENCH $args 2> >(tee "$OUT/logs/${pair}_${lane}_${side}_${run_id}.err" >&2))"
  echo "$out" > "$OUT/logs/${pair}_${lane}_${side}_${run_id}.out"
  ops="$(echo "$out" | sed -n 's/.*ops\/s=\([0-9.]\+\).*/\1/p' | head -1)"
  echo -e "${pair}\t${lane}\t${side}\t${run_id}\t${ops:-NA}" >> "$OUT/raw.tsv"
}

echo -e "pair\tlane\tside\trun\tops" > "$OUT/raw.tsv"

for pair in v1_d64 v2_d128 v3_bd16 v4_bd32 v5_d64_bd16 v6_d128_bd16; do
  for lane in main guard larson_proxy; do
    args_var="LANE_${lane}"
    for i in $(seq 1 "$RUNS"); do
      run_once "$pair" base "$lane" "$i" "${!args_var}"
      run_once "$pair" "$pair" "$lane" "$i" "${!args_var}"
      run_once "$pair" "$pair" "$lane" "$i" "${!args_var}"
      run_once "$pair" base "$lane" "$i" "${!args_var}"
    done
  done
done

echo "done: $OUT/raw.tsv"
```
