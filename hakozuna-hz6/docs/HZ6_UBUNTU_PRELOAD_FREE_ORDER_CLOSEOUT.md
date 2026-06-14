# HZ6 Ubuntu Preload Free-Order Closeout

Date: 2026-06-15

## Scope

This note closes the recent Ubuntu LD_PRELOAD free-order controls after the
page-hint closeout. The target symptom is still real on the MidPage-heavy
`4096..16384` row: selected `free()` often pays a Toy active-map miss before
the MidPage active-map hit.

Selected/default now uses a cheap active-count bias:

```text
if midpage_active_map_current > toy_small_active_map_current:
  MidPage active-map -> Toy active-map -> route
else:
  Toy active-map -> MidPage active-map -> route
```

## Lanes

| Lane | Status | Read |
| --- | --- | --- |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | control/no-go | Proves the target Toy-miss wall is removable, but badly regresses Toy/tiny and mixed guards. |
| `HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1=1` | control/no-go | Alignment is not selective enough; Toy-heavy rows also pass the MidPage alignment gate, so it behaves like unconditional MidPage-first. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1` | selected/default | Uses allocator-local active-map current counts. Balanced 1x keeps guards essentially flat and improves the MidPage-heavy target. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=2` | target upper-bound/no-go | Stronger target win, but `16..4096` regresses too much. |

## Current-Bias Evidence

The balanced current-bias shape:

```text
try MidPage first only when:
  midpage_active_map_current > toy_small_active_map_current
```

Stats repeat-5 showed the intended target path split:

```text
4096..16384:
  selected Toy attempts      ~= 1,000,608
  current-bias Toy attempts  ~= 1,914
  MidPage attempts remain    ~= 1,000,608
```

Stats-off repeat-15:

```text
16..256       58.636M -> 57.990M
16..4096      42.582M -> 42.089M
1024..4096    40.920M -> 40.688M
4096..16384   44.222M -> 45.495M
```

Ratio follow-up, stats-off repeat-9:

```text
4096..16384:
  selected 44.451M
  bias1x   44.926M
  bias2x   45.966M

16..4096:
  selected 42.242M
  bias1x   42.182M
  bias2x   40.970M
```

Selected-balance refresh, stats-off repeat-3:

```text
raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_040821

16..256      HZ6 57.981M / 30.38 MiB
16..4096     HZ6 41.201M / 79.75 MiB
1024..4096   HZ6 38.159M / 90.88 MiB
4096..16384  HZ6 42.496M / 94.50 MiB
```

Promotion repeat-15, stats-off:

```text
raw: private/raw-results/linux/hz6_current_bias_repeat15_20260615_041255

16..256      selected 58.235M -> bias1x 57.971M  (-0.45%)
16..4096     selected 42.330M -> bias1x 42.333M  (+0.01%)
1024..4096   selected 40.580M -> bias1x 40.643M  (+0.15%)
4096..16384  selected 44.163M -> bias1x 45.596M  (+3.24%)
```

Post-promotion selected repeat-5, stats-off:

```text
raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_041438

16..256      57.367M / 30.50 MiB
16..4096     42.130M / 79.62 MiB
1024..4096   39.670M / 91.00 MiB
4096..16384  46.357M / 94.50 MiB
```

Safety stats spot-check after rebuilding selected:

```text
raw: private/raw-results/linux/hz6_current_bias_selected_stats_safety_20260615_041430

16..256, 16..4096, 4096..16384:
  route_miss=0
  route_invalid=0
  alloc_fail=0
  register_fail=0
```

## Decision

Promote balanced current-bias to selected/default.

```text
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_NUMERATOR=1
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DENOMINATOR=1
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_DELTA=0
```

Keep:

```text
bias2x:
  target upper-bound evidence only
```

Do not default a stronger free-order gate until:

```text
1. the stronger gate keeps 16..256 and 16..4096 flat in repeat-15+, and
2. the stronger gate preserves the 4096..16384 current-bias win.
```

## Runner

Use:

```bash
./hakozuna-hz6/linux/run_hz6_preload_free_order_ab.sh
```

Variants:

```text
selected
midpage_first
aligned_first
current_bias
current_bias_2x
current_bias_4x
current_bias_delta64
```
