# LocalHotCodeShapeAudit-L1

Date: 2026-06-23

## Purpose

Check whether the remaining local-hot atomic fields are still paying locked RMW
costs in the release leaf before attempting a plain-scalar authority change.

Fields checked:

```text
local_free_head_word
local_bump_index
slot_state
```

## Command

```sh
cc -O2 -g -fPIC -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
  -Iinclude -Isrc -S src/h8_small_local.c -o /tmp/h8_small_local.s

rg -n "lock|xchg|cmpxchg|local_free_head_word|local_bump_index|h8_malloc_inner|h8_free_inner" \
  /tmp/h8_small_local.s
```

## Findings

The hot local alloc/free paths compile local-hot relaxed atomic operations to
ordinary loads/stores.

Freelist allocation head pop:

```asm
movl 132(%rsi), %eax
...
movl %ecx, 132(%rdx)
```

Bump allocation index update:

```asm
movl 128(%rsi), %eax
...
movl %ecx, 128(%rsi)
```

Local free head push:

```asm
movl 132(%r12), %ecx
...
movl %esi, 132(%r12)
```

Slot-state publication is also a plain store in this release shape:

```asm
movl $1073741824, (%rcx,%rax,4)
movl %eax, (%rdx)
```

The only `lock` matches in `h8_small_local.s` are slow-path mutex calls /
debug strings, not local free-head or bump-index hot operations.

## Decision

Do not prioritize `LocalFreeHeadBumpScalar-L1` as a performance box.

Changing these fields from relaxed atomics to plain scalars would not remove a
locked instruction in the measured release code shape. The next performance
work should target a different fixed cost, such as TLS access/code shape or
other local leaf instruction count.
