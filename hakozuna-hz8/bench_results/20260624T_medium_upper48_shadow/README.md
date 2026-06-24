# MediumUpper48KSizePolicyShadow-L1

## Scope

Bench attribution only.

```text
allocator behavior:
  unchanged

candidate:
  medium class map 8K / 16K / 32K / 48K / 64K

purpose:
  quantify rounded-byte benefit and run-mix impact before any behavior A/B
```

The medium attribution printer was moved from `bench/h8_bench.c` into
`bench/h8_bench_support.c` to keep the main bench file below 800 lines.

## Command

```bash
./h8_bench \
  --runs 2 \
  --threads 2 \
  --iters 2000 \
  --min-size 4097 \
  --max-size 65536 \
  --remote-pct 90 \
  --interleaved 0
```

Raw output:

```text
bench_results/20260624T_medium_upper48_shadow/medium_phase_r90_debug_short.txt
```

## Result

```text
p2 remote rounded bytes:
  335486976

upper48 remote rounded bytes:
  303456256

relative rounded bytes:
  0.9045

current run estimate:
  5121

upper48 run estimate:
  5121
```

Class split:

```text
medium p2 remote_live:
  [435,957,1927,3862]

upper48 remote_live:
  [435,957,1927,1955,1907]
```

## Interpretation

```text
48K candidate reduces rounded medium bytes by about 9.5%
48K candidate does not reduce run count with current 64KiB run geometry
```

This is a memory / first-touch candidate, not a queue-episode reduction.
Behavior A/B should only proceed if the goal is explicit RSS/rounded-byte
reduction and hot-path cost is measured against frozen defaults.
