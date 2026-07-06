# HZ10 lane cleanup smoke

Purpose: verify the lane rename/default cleanup after making fine size classes
part of the shim default.

Command:

```text
OUTDIR=hakozuna-hz10/bench_results/20260707T_lane_cleanup_smoke \
RUNS=1 \
RUN_LARSON=0 \
ALLOCATORS_CSV=hz10,hz10-coarse,hz10+fine,hz10+orphan-partial \
make -C hakozuna-hz10 bench-macro-matrix
```

Result:

```text
python_alloc hz10                  0.96s / 106,880 KiB
python_alloc hz10+fine             0.95s / 106,828 KiB
python_alloc hz10-coarse           0.96s / 116,972 KiB
python_alloc hz10+orphan-partial   0.94s / 117,024 KiB

redis_setget all rows              0.54-0.55s / 8 MiB client RSS
```

Verdict: lane cleanup is wired correctly. `hz10` and compatibility
`hz10+fine` are equivalent fine-default lanes; `hz10-coarse` and
compatibility `hz10+orphan-partial` are equivalent coarse rollback lanes.
