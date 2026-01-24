# Bench Management Rules

## Results Location

`hakozuna/hz3/results/`

```
hakozuna/hz3/results/
├── INDEX.md              # start here
└── YYYYMMDD_topic_env/   # result folders
```

## Naming

`YYYYMMDD_topic_env`

Examples:
- `20260121_s113_full_wsl`
- `20260121_paper_bench_native`
- `20260118_memcached_sweep_wsl`

## /tmp Policy

`/tmp` is temporary.

After a bench completes:
1. Move logs into `results/`
2. Update `INDEX.md`

## RUNS

WSL variance is high. Use `RUNS=10` or more and report the median.

## Current results/ layout

```
hakozuna/hz3/results/
├── INDEX.md
├── 20260118_memcached_sweep_wsl/
├── 20260120_paper_full_native/
├── 20260121_s113_vs_ptag32_native/
├── 20260121_s113_vs_ptag32_wsl/
├── 20260121_s113_full_wsl/
└── 20260121_paper_bench_wsl/
```
