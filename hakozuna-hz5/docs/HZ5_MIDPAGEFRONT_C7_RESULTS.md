# HZ5 MidPageFront C7 Results

MidPageFront-C7 is the current Linux ordinary-malloc family for `2049..32768`.
The core result is stable:

```text
same-class MidPage direct API is already tcmalloc-class
mixed-class streams lose throughput under strict classing
coarse classing recovers speed, but RSS determines promotion
```

Raw outputs:

```text
private/raw-results/linux/midpage_c7_direct_rss_sweep_20260525_172138
private/raw-results/linux/midpage_c7_preload_rss_sweep_20260525_172216
```

## Direct API

`ws=4000`, `RUNS=3`.

```text
profile        mix_3_8k ops/RSS      mix_5_32k ops/RSS
strict         53.11M / 76160 KB     30.99M / 86912 KB
band4/8/16/32  52.40M / 74368 KB     34.29M / 84096 KB
band4/8/32     59.27M / 74496 KB     33.67M / 78080 KB
band8/16/32    72.12M / 65408 KB     33.54M / 80384 KB
band8/32       71.95M / 65536 KB     33.20M / 74496 KB
wide32k        40.98M / 67328 KB     39.75M / 67328 KB
```

## Preload

`ws=4000`, `RUNS=3`.

```text
profile        row        r0 ops/RSS        r90 ops/RSS
strict         mix_3_8k   46.89M / 84864    16.29M / 422784
band8/16/32    mix_3_8k   56.42M / 81408    11.11M / 496384
band8/32       mix_3_8k   57.36M / 81280    13.13M / 447616
wide32k        mix_3_8k   36.38M / 77184    21.32M / 196096
tcmalloc       mix_3_8k   66.64M / 80128    43.32M / 140672

strict         mix_5_32k  32.03M / 91392    22.05M / 174208
band8/16/32    mix_5_32k  36.71M / 84608    19.56M / 204032
band8/32       mix_5_32k  37.18M / 78976    22.55M / 208384
wide32k        mix_5_32k  35.92M / 77312    20.49M / 208512
tcmalloc       mix_5_32k  50.96M / 84992    34.94M / 137728
```

## Interpretation

```text
Strict is the low-waste default candidate.
band8/32 and band8/16/32 are real speed/RSS candidates.
wide32k is only a speed upper-bound diagnostic.

The next implementation target is not another mapping table.
It is RSS control for coarse profiles:
  empty/mostly-empty slab release
  bytes-based cache budgets
  retention control on remote-heavy rows
```
