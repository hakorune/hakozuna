# HZ12 Windows OwnerFast RSS and Broad Controls (2026-07-10)

Status: Windows development evidence. Not a release or paper table.

## Fair Cross-Owner RSS R5

All rows used the same process-level RSS sampling, fresh binaries, rotated row
order, affinity `0xFF`, High priority, 4 producers, 4 consumers, sizes 8..1024,
and 100% cross-owner frees.

| Allocator | Median ops/s | Peak RSS median | Final RSS median |
| --- | ---: | ---: | ---: |
| HZ11 ownerless | 12.992M | **7.79 MiB** | **7.63 MiB** |
| HZ12 OwnerFastLoad | **36.427M** | 15.54 MiB | 15.39 MiB |
| tcmalloc | 36.439M | 14.81 MiB | 14.22 MiB |

HZ12 is throughput-parity in this row, but its RSS is 4.9% higher at peak and
8.2% higher at the final sample. This small working-set row does not establish
a low-RSS win over tcmalloc.

Source:
`bench_results/windows_xowner_roundrobin_ownerfast_full_rss_high_r5.md`.

## Local Random Mixed R5

The HZ12 core row does not include owner mapping or inbox behavior.

| Profile | HZ11 | HZ12 core | tcmalloc | HZ12 peak RSS |
| --- | ---: | ---: | ---: | ---: |
| small | 154.879M | 154.122M | 121.606M | 4.27 MiB |
| medium | 150.092M | 150.096M | 150.332M | 5.46 MiB |
| mixed | 150.119M | 152.453M | 149.416M | 5.57 MiB |

HZ12 core is already a strong local control and uses about half the peak RSS
of tcmalloc on these compact rows.

Source:
`bench_results/windows_broad_random_r5/20260710_202549_paper_random_mixed_windows.md`.

## MT Matrix Diagnostic R3

These legacy matrix rows complete in 0.01..0.18 seconds. Treat them as weakness
attribution, not stable headlines.

| Profile | HZ11 median | HZ12 core median | tcmalloc median | HZ12 peak RSS median |
| --- | ---: | ---: | ---: | ---: |
| balanced | 33.315M | 13.902M | 166.938M | 39.07 MiB |
| wide_ws | 20.554M | 14.632M | 88.865M | 76.98 MiB |
| larger_sizes | 35.402M | 27.734M | 58.880M | 68.26 MiB |

HZ12 core still pays the ownerless returned-list path under MT pressure. The
cross-owner inbox result shows batch routing can remove that cliff, but the
behavior is not connected to the public allocator API yet.

## Owner Attribution Decomposition

Same-owner random_mixed R5 separates allocation owner recording from a free-
time owner lookup.

| Profile | HZ12 core | Alloc map only | Alloc + free lookup |
| --- | ---: | ---: | ---: |
| small | 153.100M | 150.657M (-1.6%) | 138.505M (-10.6%) |
| medium | 151.605M | 147.970M (-2.4%) | 136.455M (-9.7%) |
| mixed | 151.430M | 147.725M (-2.4%) | 136.514M (-10.4%) |

Decision:

- GO: advisory owner recording on allocation, within the 3% local gate.
- NO-GO: owner lookup on every free.
- Next: group and route only when the local class cache flushes. Normal free
  must remain an owner-blind cache push.
- OwnerFastLoad is selected only for the Windows xowner mechanism lane. It is
  not a public/default allocator promotion yet.

Sources:

- `bench_results/windows_allocmap_local_ab_r5/20260710_203111_paper_random_mixed_windows.md`
- `bench_results/windows_ownermap_local_ab_r5/20260710_202926_paper_random_mixed_windows.md`

## Flush-Time Owner Routing L1

The integrated opt-in lane keeps normal free owner-blind. Allocation records
an advisory owner, class-cache flush groups objects by owner, and a dedicated
owner-by-class inbox returns batches to the owner cache. It does not reuse the
deferred-free inbox because those objects have already completed free.

The first reuse attempt caused recursive `hz12_free` on inbox overflow and a
Windows stack overflow. The class-aware inbox removes that unsafe lifecycle
mixing; the fixed lane completed all R5 runs.

| Allocator | Xowner median | Peak RSS median | Final RSS median |
| --- | ---: | ---: | ---: |
| HZ11 ownerless | 12.939M | **6.61 MiB** | **6.46 MiB** |
| HZ12 integrated flushroute | 26.128M | 11.79 MiB | 11.63 MiB |
| HZ12 OwnerFast upper bound | 34.324M | 15.80 MiB | 15.64 MiB |
| tcmalloc | **36.318M** | 13.81 MiB | 13.21 MiB |

Local random_mixed R5 remained about 7% below HZ12 core. An inert control
attributed roughly 2..3% to the owner-map/build skeleton and roughly 4% to the
flush-routing/drain behavior. This exceeds the 3% local acceptance gate.

Decision: HOLD as opt-in mechanism evidence. The prototype proves that
flush-granularity routing can double HZ11 ownerless cross-owner throughput
without per-free owner lookup, but it is not default quality.

Source:
`bench_results/windows_xowner_roundrobin_flushroute_compare_high_r5.md`.
