# HZ12 Windows Detached Span Decommit L2-F

## Purpose

L2-F decommits one already-detached 64 KiB span in the dedicated Windows
single-thread/quiescent smoke. It does not add a normal allocator policy,
recommit path, reuse pool, or multi-thread reclaim protocol.

## Preconditions

```text
L2-C accounting candidate is clean
L2-E ordered detach completed
front cache contains zero objects from the span
returned sink contains zero objects from the span
current span no longer references the span
direct-index route is detached
read-only reclaim gate is open
```

## Behavior

```text
VirtualQuery(span) must report MEM_COMMIT
VirtualFree(span, 64 KiB, MEM_DECOMMIT)
VirtualQuery(span) must report MEM_RESERVE
```

The 4 GiB arena reservation remains intact. Only the selected span payload is
decommitted. The direct-index route remains invalid, so stale arena free and
usable-size stay fail-closed without touching the decommitted payload.

## Acceptance

```text
gate_open == 1 before decommit
before_state == MEM_COMMIT
VirtualFree MEM_DECOMMIT succeeds
after_state == MEM_RESERVE
decommitted_bytes == 64 KiB
stale usable-size returns zero
stale free does not fault or reach system free
all prior HZ12 smoke tests remain green
```

## No-Go

```text
decommit is attempted while any detach witness remains
the arena reservation is released
route is restored without recommit
the span enters an allocation source after decommit
RSS improvement is claimed from this one-span mechanism smoke
```

## Next Gate

L2-G may add a bounded detached-span depot with explicit recommit-before-route
ordering. Until then, a decommitted span is intentionally not reusable.

## Initial Windows Evidence

The controlled full-span smoke passed the complete L2-C through L2-F sequence:

```text
span bytes = 65,536
gate_open = 1
decommit attempted = 1
decommit succeeded = 1
before_state = 0x1000 (MEM_COMMIT)
after_state = 0x2000 (MEM_RESERVE)
```

After decommit, stale usable-size returned zero and stale free remained
fail-closed. The arena reservation was not released. Owner shadow, retired
adoption, and normal xowner regression smokes remained green. A follow-up
10-run short xowner check completed with matching allocation/free counts, zero
tracked live objects, and zero accounting underflow in every run.
