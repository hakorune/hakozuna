# HZ5 Linux Route / Lane Matrix

This file is the current source of truth for Linux HZ5 route and lane names.
Historical lane detail is archived at:

```text
hakozuna-hz5/docs/archive/HZ5_LINUX_ROUTE_LANE_MATRIX_HISTORY_2026-05.md
```

For day-to-day work, read these first:

```text
hakozuna-hz5/docs/HZ5_LINUX_DEV_BRIEF.md
hakozuna-hz5/docs/HZ5_LINUX_LANE_COMBINATIONS.md
current_task.md
```

## Naming Rule

```text
Route:
  the memory path actually used by one allocation/free

Lane:
  a build/profile name selected by compile flags

Benchmark profile:
  the workload family used to judge a lane
```

Do not use a route name as a paper claim. Report the exact lane, benchmark
profile, run root, and comparison allocator.

## Reporting Rows

| Reporting row | Lane family | Status |
| --- | --- | --- |
| Exact 64K/a8192 local/mixed speed | `local2p-linkflags` | accepted appendix profile |
| Exact retained RSS throughput | `local2p-rssretain2048tls` | accepted appendix profile |
| Exact producer/consumer remote free | `local2p-remotebatch` | accepted appendix profile |
| General ordinary malloc baseline | `general-region-outbox` | broad baseline/control |
| PageRun64 main/mid ordinary malloc | `pagerun64-main` | current general candidate |
| Cross-size 128K remote-heavy | `pagerun64-cross128` | saved fixed profile |
| Large128 low-RSS remote-heavy | `large128-rss` | saved fixed profile |
| Large128 source-batch16 throughput | `large128-source16` | comparison diagnostic |
| Large128 r50 drain family | `large128-r50-*` | diagnostic only |
| Large128 drain implementation | `large128-drainbulk` | diagnostic only |
| Large128 drain state transition | `large128-draintrust` | diagnostic only |
| Large128 policy family | `large128-policy-*` | observation/diagnostic only |

## Active Route Families

| Route family | Domain | Current role |
| --- | --- | --- |
| `smallfront_s1` | ordinary malloc `<= 2048` | active small front-end |
| `midfront_m1` | ordinary malloc `2049..65536` legacy/control | control and fallback comparison |
| `midpagefront_pagerun64` | ordinary malloc `2049..32768` | current PageRun64 mid route |
| `largefront_l1/l2` | ordinary malloc `> 64K..1M` | active large route |
| `local2p` | exact `64K/a8192` | appendix/specialized route |
| `p25_bridge` | exact/control bridge | legacy baseline |
| `libc_passthrough` | bootstrap/unsupported | attribution control, never a HZ5 win |

## Current Large128 Lane Policy

Prefer human-facing aliases in new commands:

```text
hz5-large128-rss
hz5-large128-source16
hz5-large128-r50-drain
hz5-large128-drainbulk
hz5-large128-draintrust
hz5-large128-r50-hold
hz5-large128-r50-hold8
hz5-large128-policy-l8-shadow
```

Keep historical `hz5-pagerun64-large128-*` names only for reproducing older
result directories.

Promotion rules:

```text
large128-rss:
  saved low-RSS profile

large128-source16:
  comparison lane for throughput and perf inspection

large128-r50-*:
  no broad promotion until it beats source16 or large128-rss on the focused
  large128 matrix without losing r90/RSS

large128-drainbulk:
  diagnostic only; t4 signal, t8 regression

large128-draintrust:
  diagnostic only; t8/r50 signal, r90 regression

large128-policy-*:
  observation or diagnostic only; no hot-path counters in speed lanes
```

## Diagnostics Not To Promote

| Diagnostic | Reason |
| --- | --- |
| `direct-header` | unsafe adjacent-header lookup upper bound only |
| `source-populate` | hard no-go; destroys throughput and RSS |
| `ownerfast` | no-go in focused large128 check |
| `rb32/rb64` | high-thread signal but unstable across t4 rows |
| `batch32` | source overfetch no-go |
| `remotehold4/8` | useful evidence but not broad replacement |
| `global-remote` | slower than owner inbox/source profiles |

## Update Discipline

When adding a lane:

```text
1. Add build alias only if the lane is reproducible.
2. Add runner allocator only if it will be measured repeatedly.
3. Record no-go lanes in HZ5_LINUX_LANE_COMBINATIONS.md or current_task.md.
4. Keep full historical result narratives in archive docs, not this file.
```
