# Current Task

This root file is the short project orientation ledger. Do not add chronological
benchmark logs here.

## Active Work

```text
Primary family:
  HZ8 allocator development and benchmarking

Current direction:
  Keep HZ8 MediumRun-v1.1 frozen as the balanced default.
  HZ8-v2 throughput work is the next research box.
  Treat the narrow HZ6 Windows appcap-only baseline as frozen reference
  evidence, not a current promotion target.

Current strength:
  HZ8 is the balanced default line.
  HZ6 narrow Windows baselines are frozen as reference evidence.
  The next value gain is expected from HZ8-v2 throughput work, not from
  reopening HZ6 control rows.
```

## Read First

```text
HZ8 active orientation:
  hakozuna-hz8/current_task.md
  hakozuna-hz8/README.md
  hakozuna-hz8/README.ja.md
  hakozuna-hz8/docs/HZ8_V2_HZ9_DESIGN.md
  hakozuna-hz8/docs/HZ8_BENCH_GATE.md

Frozen HZ6 reference:
  docs/benchmarks/windows/paper/20260629_045702_paper_larson_windows.md
  hakozuna-hz6/docs/current_task.md
```

## Archive Map

```text
Root archive index:
  docs/archive/README.md

HZ6 archive index:
  hakozuna-hz6/docs/archive/README.md

Detailed historical ledgers:
  docs/archive/current_task_2026-06-hz6_linux_history.md
  hakozuna-hz6/docs/archive/current_task_2026-06_history.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_pre_compaction.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_profile_quiescent_snapshot.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_post_toytrusted_controls_snapshot.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_adaptive_profile_snapshot.md
  hakozuna-hz6/docs/archive/current_task_2026-06-16_calloc_profile_snapshot.md
```

## Rules

```text
Keep this file below 80 lines.
Put HZ6 decisions in hakozuna-hz6/docs/current_task.md or stable HZ6 docs.
Move long benchmark logs and chronological notes to archive/.
Large archived ledgers may exceed 3000 lines; active current_task files should
not.
Do not promote profile-only lanes into selected/default without focused,
fixed, stats/diagnostic, RSS, and cross-allocator guard evidence.
```
