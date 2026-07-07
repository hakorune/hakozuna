# HZ10 Bump Init L0, RUNS=10

Status: `GO(opt-in measurement)` at the time of this L0 run.

Follow-up: promoted to default by
[`HZ10 bump default gate L1`](../20260707_hz10_bump_default_gate_l1/README.md)
after macro, sanitizer, RSS-guard, and steady-state gates.

`HZ10_ENABLE_BUMP_INIT=1` changes fresh-page initialization from eager
intrusive freelist construction to lazy bump allocation. It is exposed as the
`hz10_bump` preload sibling (`hakozuna-hz10/libhz10_bump.so`) in this run. It is
now the default HZ10 behavior; `libhz10_nobump.so` is the rollback lane.

## Conditions

```text
date_utc=2026-07-07
RUNS=10
THREADS=16
ITERS=50000
allocators=hz10,hz10_bump,hz8,tcmalloc
rows=guard_local0,main_local0,main_interleaved_r50,main_interleaved_r90,small_interleaved_remote90,medium_interleaved_r50
harness=hakozuna-hz8/bench/bench_matrix_malloc via LD_PRELOAD
raw=samples.csv
summary=summary.md
```

## HZ10 vs Bump

| row | hz10 ops/s | hz10_bump ops/s | speed ratio | hz10 post RSS | hz10_bump post RSS | minor faults |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `guard_local0` | 132.09M | 238.52M | 1.81x | 26.38MiB | 3.25MiB | 6510 -> 624 |
| `main_local0` | 113.91M | 207.23M | 1.82x | 33.75MiB | 4.88MiB | 8322 -> 1010 |
| `main_interleaved_r50` | 20.36M | 21.61M | 1.06x | 76.88MiB | 64.00MiB | 19366 -> 16229 |
| `main_interleaved_r90` | 12.77M | 13.12M | 1.03x | 85.25MiB | 71.06MiB | 21573 -> 18030 |
| `small_interleaved_remote90` | 14.92M | 15.42M | 1.03x | 43.00MiB | 35.06MiB | 10720 -> 8772 |
| `medium_interleaved_r50` | 19.21M | 19.22M | 1.00x | 65.75MiB | 65.06MiB | 16547 -> 16447 |

## Comparator Read

| row | hz8 ops/s / post RSS | hz10_bump ops/s / post RSS | tcmalloc ops/s / post RSS |
| --- | ---: | ---: | ---: |
| `guard_local0` | 221.90M / 1.94MiB | 238.52M / 3.25MiB | 371.63M / 7.00MiB |
| `main_local0` | 121.43M / 3.38MiB | 207.23M / 4.88MiB | 429.76M / 9.00MiB |
| `main_interleaved_r50` | 11.04M / 4.27MiB | 21.61M / 64.00MiB | 22.44M / 65.19MiB |
| `main_interleaved_r90` | 6.95M / 4.56MiB | 13.12M / 71.06MiB | 13.87M / 87.38MiB |
| `small_interleaved_remote90` | 14.76M / 2.88MiB | 15.42M / 35.06MiB | 26.19M / 30.19MiB |
| `medium_interleaved_r50` | 9.71M / 3.88MiB | 19.22M / 65.06MiB | 16.88M / 77.94MiB |

## Verification

- Default HZ10: `make -B smoke-public-entry hz10-standalone-check` passed.
- Bump-enabled HZ10: `smoke-public-entry`, `smoke-public-entry-orphan-partial`,
  `smoke-public-entry-retired-local`, and `hz10-standalone-check` passed with
  `HZ10_ENABLE_BUMP_INIT=1`.
- Bump-enabled TSan smoke passed through `smoke-tsan-aslr-off`.
- Bump-enabled ASan/UBSan public-entry, orphan-partial, and retired-local
  smoke passed with `ASAN_OPTIONS=detect_leaks=0`.

## Verdict

`HZ10_ENABLE_BUMP_INIT=1` fixed the HZ10 local fixed-cost weakness relative to
HZ8 on these rows and cut page-fault pressure substantially. It also improved
post RSS on the measured HZ10 rows because unused slots are not eagerly
faulted/touched.

The follow-up L1 gate made it default. The notable corrected invariant is that
`free_count` still counts never-bumped slots as available capacity; active
scans, retired harvest, and partial orphan adoption must use
`hz10_freelist_page_has_capacity()` rather than checking only
`local_free_head`.
