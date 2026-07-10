# HZ12 Windows Measurement Table (2026-07-10)

Status: development snapshot, not a paper or release table.

This table keeps directly comparable measurements separate from lifecycle
diagnostics. It does not combine Linux HZ11 fine128 results, Windows allocator
matrix rows, and the HZ12 wrapper pipeline as if they were one benchmark.

## Fresh Cross-Owner Comparison

Same-session Windows run, `RUNS=5`, 5 seconds per run, 4 producers, 4
consumers, ring 4096, sizes 8..1024, and 100% cross-owner frees.

| Allocator | Median ops/s | vs HZ11 | vs tcmalloc |
| --- | ---: | ---: | ---: |
| HZ11 ownerless | 5.828M | 1.00x | 0.212x |
| HZ12 owner-inbox L1 | 10.634M | **1.82x** | 0.388x |
| tcmalloc | 27.440M | 4.71x | 1.00x |

Reading:

- HZ12 materially improves the exact HZ11 ownerless cross-owner control.
- HZ12 remains about 61% below tcmalloc in this fresh run.
- This supports "HZ11 successor mechanism" but not "complete HZ11 superset."

Source: `bench_results/windows_hz11_hz12_tcmalloc_xowner_r5.md`.

This grouped table is retained as measurement-drift evidence only. It is not
the current headline because allocator order was not rotated.

## Provenance-Safe Round-Robin Comparison

Fresh builds, `RUNS=5`, five seconds per row, rotated row order, affinity
`0xFF`, High process priority, and the same 4P/4C 100% cross-owner workload:

| Allocator | Median ops/s | vs HZ11 | vs tcmalloc |
| --- | ---: | ---: | ---: |
| HZ11 ownerless | 13.046M | 1.00x | 0.354x |
| HZ12 owner-inbox speed | **28.896M** | **2.21x** | **0.784x** |
| tcmalloc | 36.833M | 2.82x | 1.00x |

The HZ12 speed row preserves owner mapping, bounded mutex inbox publication,
owner drain, ownerless fallback, and retirement behavior. Atomic whole-span
accounting and shadow/inbox diagnostic counters are compile-time disabled.

Source:
`bench_results/windows_xowner_roundrobin_speed_compare_high_r5.md`.

## Diagnostic-Tax A/B

| A/B | A median | B median | Delta |
| --- | ---: | ---: | ---: |
| diagnostic L1 -> accounting off | 18.420M | 25.351M | **+37.6%** |
| accounting off -> counters off | 25.620M | 28.477M | **+11.2%** |

Both deltas exceed the corrected runner's approximately 4% A/A median-position
noise. Accounting and counters are therefore confirmed measurement taxes, not
intrinsic owner-inbox costs.

Sources:

- `bench_results/windows_xowner_roundrobin_accounting_ab_high_r5.md`
- `bench_results/windows_xowner_roundrobin_counter_ab_high_r5.md`

## Owner-Inbox Mechanism Ceiling

| Row | Median ops/s | Ratio |
| --- | ---: | ---: |
| HZ12 bare ownerless core | 13.434M | 1.00x |
| HZ12 owner-inbox speed | **29.274M** | **2.18x** |

The bare control uses the same harness and HZ12 core but calls `hz12_free`
directly on the consumer. It therefore pays the core's per-object ownerless
returned-list path. The result proves that bounded batch inbox handoff is a
net performance mechanism, not the remaining bottleneck as a whole.

Source: `bench_results/windows_xowner_roundrobin_routing_ab_high_r5.md`.

## Owner-Map Fast-Load Candidate

The candidate first performs a relaxed owner-token load. An already matching
owner returns without a locked operation; an unowned span still uses the
first-writer CAS, and a foreign token preserves existing routing semantics.

| Comparison | Control | Candidate | Candidate delta |
| --- | ---: | ---: | ---: |
| owner-map A/B | 29.292M | **36.052M** | **+23.1%** |
| candidate vs tcmalloc | 37.597M tcmalloc | 35.542M HZ12 | **-5.5%** |

The candidate reaches 94.5% of tcmalloc in this exact 100% cross-owner row.
Repeat-10 short safety runs completed with zero retired pending owners/objects.
This is a Windows xowner candidate, not a broad/default promotion.

Sources:

- `bench_results/windows_xowner_roundrobin_ownermap_ab_high_r5.md`
- `bench_results/windows_xowner_roundrobin_ownerfast_tcmalloc_high_r5.md`

## Earlier Same-Session R3

The earlier run used the same workload shape but produced a different absolute
result:

| Allocator | Earlier median | Fresh median | Fresh / earlier |
| --- | ---: | ---: | ---: |
| HZ11 ownerless | 9.138M | 5.828M | 0.638x |
| HZ12 owner-inbox L1 | 24.516M | 10.634M | 0.434x |
| tcmalloc | 28.553M | 27.440M | 0.961x |

The allocator-specific drift is too large to treat either result as a stable
headline. A later alternating null control measured the preserved early HZ12
binary at 18.538M and the current binary at 18.937M, so the source rebuild did
not reproduce the grouped R5 collapse. The next promotion table must rotate
allocator order and record commit, binary hash, affinity, and power state.

## HZ12 Inbox RSS Control

Existing cap sweep, same 4P/4C cross-owner shape, `RUNS=3`:

| Inbox cap | Median ops/s | Peak RSS | Final RSS | Overflow objects |
| ---: | ---: | ---: | ---: | ---: |
| 512 | 24.217M | 18.32 MiB | 18.18 MiB | 281,076 |
| 1024 | 24.612M | **17.04 MiB** | **16.89 MiB** | 4,007 |
| 2048 | 24.680M | 18.01 MiB | 17.86 MiB | 256 |

These RSS values are HZ12 policy evidence from the earlier campaign. That
campaign did not capture directly comparable HZ11/tcmalloc RSS, so this table
must not be used for a cross-allocator RSS claim.

## Reclaim Lifecycle Evidence

The wide_ws-like diagnostic is structural evidence, not a speed row.

| Gate | Result |
| --- | ---: |
| Full-span physical candidates, L5-B median | 80.75 MiB |
| Fixed bounded detach | 64 spans / 4.00 MiB |
| Fixed bounded decommit | 64 spans / 4.00 MiB |
| Observed working-set RSS reduction | 3.99 MiB per run |
| Recommit and same-address restore | 64/64 |
| Full-slot touch/free and second reclaim | 64/64 |
| Address / generation / lifecycle failures | 0 |

This proves that HZ12 can recover RSS and reuse the spans safely. It does not
yet show the throughput/RSS result of an automatic reclaim policy.

## Superset Gate

| Area | Current evidence | Verdict |
| --- | --- | --- |
| 100% cross-owner pipeline | speed HZ12 2.21x HZ11, 78.4% of tcmalloc | HZ12 wins HZ11 control |
| Bounded span reclaim safety | Full two-cycle proof | HZ12 adds capability |
| Local/random throughput | No current same-binary HZ11/HZ12 table | Open |
| Mixed/wide throughput | HZ12 run is diagnostic, not speed-valid | Open |
| Automatic reclaim RSS/throughput | Not connected | Open |
| Real applications | No HZ12 Windows comparison | Open |

Current conclusion: HZ12 is an architectural successor and a cross-owner plus
reclaim capability superset. It is not yet a demonstrated performance superset
of HZ11.

The source/assembly audit also found at least seven locked RMWs per tracked
HZ12 cross-owner operation versus no locked RMW on a tcmalloc thread-cache hit.
See `HZ12_WINDOWS_TCMALLOC_HOTPATH_AUDIT_20260710.md`.

## Next Measurement Set

1. Add a bare HZ12 core ceiling row to the corrected round-robin runner.
2. Separate owner-map CAS/lookup from grouping/drain only if the speed lane is
   materially below bare core.
3. Connect HZ11, the HZ12 speed row, and tcmalloc to the same Windows allocator
   matrix for `balanced`, `wide_ws`, and `larger_sizes`.
4. Run Windows `random_mixed` local small/medium/mixed rows.
5. Add owner-local batch-ledger reclaim authority only as a sibling lane, then compare throughput and
   peak/final RSS against the counter-free HZ12 control.
6. Run real applications only after the synthetic matrix has no correctness or
   broad-regression blocker.
