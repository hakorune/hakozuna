# HZ6 Ubuntu Preload Free-Order Closeout

Date: 2026-06-15

## Scope

This note closes the recent Ubuntu LD_PRELOAD free-order controls after the
page-hint closeout. The target symptom is still real on the MidPage-heavy
`4096..16384` row: selected `free()` often pays a Toy active-map miss before
the MidPage active-map hit.

Selected/default remains:

```text
Toy active-map -> MidPage active-map -> route
```

## Lanes

| Lane | Status | Read |
| --- | --- | --- |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | control/no-go | Proves the target Toy-miss wall is removable, but badly regresses Toy/tiny and mixed guards. |
| `HZ6_PRELOAD_FREE_MIDPAGE_ALIGNED_FIRST_L1=1` | control/no-go | Alignment is not selective enough; Toy-heavy rows also pass the MidPage alignment gate, so it behaves like unconditional MidPage-first. |
| `HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=1` | target-positive control/watch | Uses allocator-local active-map current counts. Balanced 1x is target-positive but still slightly weak on small guards. |
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

## Decision

Keep current-bias off in selected/default for now.

```text
HZ6_PRELOAD_FREE_MIDPAGE_CURRENT_BIAS_FIRST_L1=0
```

Keep:

```text
bias1x:
  balanced control/watch

bias2x:
  target upper-bound evidence only
```

Do not default another free-order gate until either:

```text
1. bias1x passes a broader selected-balance matrix, or
2. a cheaper guard isolates small/Toy rows without losing the 4096..16384 win.
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
