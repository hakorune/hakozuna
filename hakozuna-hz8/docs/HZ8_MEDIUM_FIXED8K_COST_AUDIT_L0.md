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
