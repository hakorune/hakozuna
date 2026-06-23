# LocalFreeHeadSentinelUnify-L1

Date: 2026-06-23

## Scope

Unify the local free-list sentinel with the tagged slot-state payload sentinel.

Before this box:

```text
local_free_head_word empty:
  UINT32_MAX

FREE(next) payload empty:
  H8_SLOT_NONE == 0x3fffffff
```

That forced conversion on freelist pop and local/remote free.

After this box:

```text
local_free_head_word empty:
  H8_SLOT_NONE

FREE(next) payload:
  same representation
```

Slot count is <= 4096, so `H8_SLOT_NONE` is outside the valid slot range.

## Code Shape

Malloc freelist pop no longer needs sentinel conversion:

```asm
cmp    $0x3fffffff,%eax
je     bump_path
mov    (%rcx,%rax,4),%ecx
and    $0x3fffffff,%ecx
mov    %ecx,local_free_head
```

Local free no longer converts `UINT32_MAX` to `H8_SLOT_NONE` before publishing
`FREE(next)`:

```asm
mov    local_free_head,%edx
or     $0x80000000,%edx
mov    %edx,(slot_state)
mov    slot,local_free_head
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
  median 394.81M ops/s
  p25    394.81M ops/s
  min    371.94M ops/s
  steady median 463.28M ops/s

small_interleaved_remote90:
  median 45.81M ops/s
  p25    45.81M ops/s
  min    43.60M ops/s
  steady median 49.29M ops/s
```

Interleaved R3 remains noisy but above the v0 bring-up gate.

## Decision

Keep the change.

It is a monomorphic representation cleanup with a direct instruction-shape win
and no protocol change.
