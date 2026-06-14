# HZ6 Ubuntu Preload Free-Hint Closeout

Date: 2026-06-14

## Scope

This note closes the Ubuntu LD_PRELOAD selective MidPage-first free ordering
lane.  The target symptom was clear: on the MidPage-heavy 4096..16384 row,
preload `free()` spent many calls on a Toy active-map miss before the MidPage
active-map hit.

## Baseline Evidence

`HZ6_PRELOAD_HOOK_DETAIL` showed the original target pressure:

```text
4096..16384:
  free_toy_active_map_attempt ~= 406801
  free_toy_active_map_hit     ~= 39
  free_toy_active_map_miss    ~= 406762
  free_midpage_active_map_hit ~= 405883
```

Unconditional `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` proved that removing the
Toy-miss wall can help the target, but it badly regressed Toy/tiny guard rows.
That shape remains a control/no-go.

## Lanes

| Lane | Status | Read |
| --- | --- | --- |
| `HZ6_PRELOAD_FREE_MIDPAGE_FIRST_L1=1` | control/no-go | Target-positive, guard-negative. Keep off. |
| `HZ6_PRELOAD_FREE_MIDPAGE_HINT_DRYRUN_L1=1` | diagnostic/no-go direction | Recent min/max envelope had high target coverage but huge guard false positives. Do not behaviorize. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1` | diagnostic-only | Page table dry-run was much cleaner than the broad envelope, but still requires a per-free hint probe. Keep as attribution evidence. |
| `HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1` | control/no-go | Page-hinted behavior reduced target Toy probes but regressed every focused short repeat-5 row. Keep off. |

## Page-Hint Dry-Run Result

Build:

```text
HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_DRYRUN_L1=1
HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_CAPACITY=32768
```

Short 200K read:

```text
16..4096:
  mh_would=564
  mh_true=31
  mh_false=533
  mh_miss=0

1024..4096:
  mh_would=891
  mh_true=40
  mh_false=851
  mh_miss=0

4096..16384:
  mh_would=363037
  mh_true=362303
  mh_false=31
  mh_miss=43618
```

The page-hint table removed the broad-envelope false-positive wall:

```text
recent min/max false positives:
  16..4096    ~= 251906
  1024..4096  ~= 260851

page-hint false positives:
  16..4096    = 533
  1024..4096  = 851
```

However, this evidence is diagnostic only.  A free-time table probe is still
paid on every `free()` in the behavior control.

## Behavior A/B Result

Build:

```text
HZ6_PRELOAD_FREE_MIDPAGE_PAGE_HINT_FIRST_L1=1
```

Short repeat-5, stats off, versus selected default:

```text
16..256       47.155M -> 44.892M
16..4096      27.760M -> 26.355M
1024..4096    25.563M -> 24.662M
4096..16384   26.146M -> 25.194M
```

The behavior counter read confirmed that the intended path reduction happened:

```text
4096..16384:
  free_toy_active_map_attempt ~= 33740
  mh_true                     ~= 373061
  mh_false                    ~= 29
  mh_miss                     ~= 32763
```

Despite that path reduction, throughput fell.  The likely dominant cost is the
all-free page-hint probe/table access plus extra branch pressure, not the
remaining Toy-miss path.

## Decision

Keep all selective MidPage-first free hint behavior off by default.

Selected/default remains:

```text
Toy active-map -> MidPage active-map -> route
```

Do not promote any per-free page-hint behavior gate from this lane.

## Next Direction

Further work should avoid adding a new free-time classification probe.  Prefer
one of:

```text
1. Decision data already available on the hot free path.
2. A narrower allocation/front-boundary bit that avoids probing on every free.
3. A source-run/class hint attached to existing active-map or descriptor flows.
```

The useful lesson is that selective ordering is only worth pursuing if the
classification cost is effectively free.
