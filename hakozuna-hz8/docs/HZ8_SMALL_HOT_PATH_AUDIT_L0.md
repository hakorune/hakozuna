# HZ8 Small Hot-Path Audit L0

## Question

HZ8's public default remains materially behind tcmalloc on local small/broad
rows even after medium substrate promotion and recovery-policy closeout. This
audit asks whether the warmed same-owner small path is dominated by removable
entry cost or by the required fail-closed contract.

## Scope

```text
sizes:
  64B / 128B / 256B

allocators:
  HZ8 public default
  HZ11 fine128 speed substrate
  tcmalloc

shape:
  one pinned thread
  one live object: alloc -> touch first/last byte -> free
  warmed before measurement
  fresh process per sample
  allocator order rotated each run
  identical HZ8 A/A siblings mixed into the rotation
```

Measured binaries are release builds. No diagnostic counter, atomic, or
shadow accounting is compiled into their malloc/free paths. Static
disassembly is saved separately and cannot by itself establish executed cost.

## Outputs

```powershell
.\hakozuna-hz8\scripts\audit_small_hotpath_windows.ps1 -Runs 5
```

The command writes:

- exact build flags;
- executable hashes and compiler identity;
- per-run and median nanoseconds/process-cycles per pair;
- HZ8 A/A median separation;
- HZ8 and HZ11 entry disassembly and symbol lists;
- static instruction/call/locked-RMW summaries.

## Gate

```text
SmallEntryTrim-L1:
  open only if removable decode/call/layout work explains >= 20% of HZ8 cost

substrate evidence:
  record if HZ11 recovers >= 30% while preserving a comparable warmed shape
  do not merge HZ11 ownership or transfer semantics into HZ8 from this audit

contract floor:
  if required route/owner/generation/state work dominates, close local parity
  pursuit and retain HZ8's balanced low-RSS positioning

no-go:
  no counter in a speed binary
  no headline decision when HZ8 A/A separation exceeds 3%
  no new cache/tier before attribution
  no weakening MISS / VALID / INVALID or generation checks
  no default promotion from this diagnostic audit
```

## Status

```text
L0 implementation: complete
behavior change: none
public default: unchanged
SmallEntryTrim-L1: NO-GO
next attribution surface: mixed working-set transitions/refill
```

## Windows Result

Windows x86-64, clang-cl 18.1.8, release builds, one pinned thread, 20
million warmed alloc/touch/free pairs per sample. Allocator and size order were
rotated over R10. `hz8-a` and `hz8-b` are labels for the exact same executable.

| Size | HZ8 pooled cycles/pair | HZ11 fine128 | tcmalloc | HZ8 vs tcmalloc | HZ11 saving vs HZ8 |
|---:|---:|---:|---:|---:|---:|
| 64 B | 20.06 | 10.99 | 24.30 | 17.43% fewer | 45.22% |
| 128 B | 20.07 | 10.97 | 24.28 | 17.34% fewer | 45.32% |
| 256 B | 22.41 | 11.10 | 24.30 | 7.78% fewer | 50.48% |

The HZ8 A/A median separation was 0.81% at 64 B and 0.21% at 128 B.
The 256 B samples were bimodal and the two labels separated by 8.53%, so that
row is directional evidence only and is not a headline result.

Static whole-function shapes were:

| Symbol | Instructions | Calls | Locked RMW |
|---|---:|---:|---:|
| `h8_malloc_inner` | 207 | 9 | 0 |
| `h8_free_inner` | 155 | 11 | 0 |
| `hz11_malloc` | 84 | 3 | 0 |
| `hz11_free` | 42 | 0 | 0 |

These counts include cold blocks. They explain why HZ11 is useful as a lean
substrate bound, but they do not justify deleting HZ8 route, owner, generation,
or slot-state checks.

## Decision

HZ8's warmed same-owner small pair is not the measured public-matrix blocker.
At the two noise-valid sizes it is already about 17% cheaper than tcmalloc.
The remaining HZ8/tcmalloc gap in small and broad workloads therefore belongs
to working-set transitions, refill, sharing, or retention rather than the
steady active-span pair.

HZ11 shows that a weaker-contract substrate can save about 45% of the pair
cost, but this audit does not identify 20% removable work that preserves the
HZ8 contract. `SmallEntryTrim-L1` is closed. Do not add a trusted-free shortcut
or another small cache from this result.

The next diagnostic may reuse the existing diagnostic-only counters for active
hits, magazine pop/hit/reject, class-attributed empty pops, span commits, and
commit time. No new production counter is required.
