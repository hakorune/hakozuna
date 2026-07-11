# HZ8 Medium Fixed 8K Cost Audit L0

## Question

The active-run local fast-tier family is closed. Near-perfect cache reuse still
lost 2-4% across the current Windows gate, so another cache in front of the HZ8
medium path is not justified. This audit asks what remains in the common fixed
8K alloc/free path:

```text
removable fixed cost:
  decode, duplicated validation, calls, branch/layout, redundant loads

contract cost:
  fail-closed identity validation, state authority, pending/owner checks,
  required state publication
```

## Rules

- diagnostic objects use the same release flags as the Windows HZ8 default;
- production malloc/free receives no counter, atomic, or instrumentation;
- static instruction counts do not substitute for cycle evidence;
- no validation is removed before it is classified as redundant or required;
- compare the existing HZ10 substrate before inventing another allocator line.

## Tool

```powershell
.\hakozuna-hz8\scripts\audit_medium_fixed8k_windows.ps1
```

The tool writes:

- exact build manifest;
- symbol tables and disassembly for medium alloc/free objects;
- per-symbol instruction, call, and locked-RMW counts;
- fixed 8K repeat throughput for available HZ8, HZ10, and tcmalloc rows.

## Decision

```text
removable fixed cost >= 30% of the audited path:
  open one HZ8 common-entry trim box

contract cost dominates:
  do not weaken safety incrementally
  evaluate an HZ10-style intrusive page / O(1) pagemap substrate through an
  HZ8-native shadow contract

unclear or layout-sensitive:
  add a cycle-level microbench before behavior work
```

The audit is attribution only. It cannot promote a default behavior by itself.

## Windows L0 Result

Release-equivalent object audit, fixed 8K, four threads, one million logical
operations per thread, repeat-5:

| Allocator | Median ops/s | Median process cycles/op |
|---|---:|---:|
| HZ8 v2 | 65.447M | 258.09 |
| tcmalloc | 213.284M | 76.60 |

Static HZ8 function shape:

| Symbol | Instructions | Calls | Locked RMW |
|---|---:|---:|---:|
| `h8_medium_malloc_class_inner` | 315 | 36 | 0 |
| `h8_medium_free_inner` | 217 | 18 | 0 |
| `h8_medium_run_alloc_local_scaffold` | 64 | 1 | 0 |
| `h8_medium_run_free_local_scaffold` | 126 | 1 | 0 |

The cycle gap is real, while locked RMW is absent from these objects. The
remaining static bodies include both hot and cold blocks, so their total size
does not prove that 30% is removable. It does prove that another reuse cache is
the wrong next experiment: fixed 8K already reaches the existing active-run
path and still pays a large common-entry / validation / state-transition body.

## L0 Decision

```text
GO:
  hot/cold basic-block split and source-to-assembly classification

HOLD:
  common-entry behavior trim until the executed hot block is isolated

NO-GO:
  another cache/tier in front of the HZ8 medium path
  atomic/lock contention tuning as the primary fixed-8K explanation
```

The next audit should identify the executed active-run allocation and
same-owner free blocks, then classify each load/check/write as redundant shape
or required contract. If required contract dominates, proceed to the existing
HZ10 substrate shadow rather than weakening HZ8 safety.

The audit tool now builds that HZ10 substrate as a private sibling using the
same Windows `bench_allocator_compare` source. It is not registered in the
normal allocator matrix and does not change either allocator. Its purpose is
to measure whether the existing intrusive-page/O(1)-pagemap shape provides the
missing fixed-8K cost boundary before any HZ8 contract-import design begins.

## HZ10 Substrate Shadow Result

Windows repeat-3, same fixed 8K runner:

| Allocator | Median ops/s | Median process cycles/op | Relative to HZ8 |
|---|---:|---:|---:|
| HZ8 v2 | 66.105M | 254.50 | 1.00x |
| HZ10 substrate shadow | 184.485M | 94.74 | 2.79x |
| tcmalloc | 216.546M | 75.10 | 3.28x |

The existing HZ10 shape recovers most of the fixed-8K gap without an HZ8
common-entry trim. It reaches about 85% of tcmalloc throughput and reduces the
cycle cost by about 63% relative to HZ8. This exceeds the 30% substrate gate by
a wide margin.

Decision:

```text
common-entry trim:
  CLOSED as the primary track

HZ10 substrate shadow:
  GO as architecture evidence

direct merge/default promotion:
  NOT AUTHORIZED
```

The HZ10 shadow does not yet carry the complete HZ8 route, ownership,
remote-pending, owner-exit, or low-post-RSS contract. The next box is therefore
a contract-delta design, not a source copy or default lane.
