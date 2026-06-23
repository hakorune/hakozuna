# MallocSlotStatePointerCache-L1

Date: 2026-06-23

## Scope

Cache the slot-state pointer inside the malloc freelist-pop path.

This is a code-shape box.  It does not change slot-state authority, pending
bitmap authority, ownership, or remote publish protocol.

## Worker Evidence

The malloc-side worker found that freelist-pop allocation loaded
`span->slot_state` twice in the same inlined body:

```text
load slot_state pointer
load FREE(next)
...
load slot_state pointer again
store ALLOCATED
```

## Change

Add pointer-taking hot helpers:

```text
h8_slot_state_load_ptr_hot
h8_slot_state_store_allocated_ptr_hot
```

Use them only in the malloc freelist-pop path after caching:

```c
_Atomic uint32_t* slot_state = span->slot_state;
```

## Code Shape

The freelist-pop path now computes the slot-state entry address once and uses
the same entry pointer for load and store:

```asm
mov    0x20(%rdx),%rcx
lea    (%rcx,%rax,4),%rsi
mov    (%rsi),%ecx
and    $0x3fffffff,%ecx
mov    %ecx,local_free_head
movl   $0x40000000,(%rsi)
```

## Checks

```text
make bench-release smoke safety-stress
./h8_smoke
./h8_safety_stress
```

Result: pass.

Focused RUNS=3 checks:

```text
guard/local0:
  median 390.06M ops/s
  p25    390.06M ops/s
  min    344.24M ops/s
  steady median 468.59M ops/s

small_interleaved_remote90:
  median 43.37M ops/s
  p25    43.37M ops/s
  min    42.69M ops/s
  steady median 49.18M ops/s
```

R3 remains a sanity check only.  Both lanes remain above bring-up gates.

## Decision

Keep the change.

Next candidate from the worker sweep is remote-side:

```text
RemotePublishLockedInline-AB-L1
```

That must remain an A/B because it can increase text size even though it does
not change the remote protocol.
