# HZ12 Windows Token Xowner Pipeline L4-B

## Scope

L4-B runs a one-producer/one-consumer, 100% cross-owner pipeline. Pointer
ownership is still derived at the flush boundary from the existing span shadow,
but batches publish through the `slot + generation` token inbox.

## Lifecycle

```text
ACTIVE:
  producer allocates and periodically drains token inbox
  consumer batches foreign frees and token-publishes at flush

RETIRING:
  producer stops allocation, starts owner epoch, and checkpoints
  consumer flushes its final batch and checkpoints

DEAD:
  L3-D gate must see both acknowledgements and an empty inbox
  owner marks DEAD only after the final drain
```

## Boundary

This is a dedicated diagnostic runner. Normal `hz12_free` still performs no
owner token lookup. The runner measures a controlled single-owner pipeline,
not a multi-owner production profile.

## Acceptance

```text
100% foreign free pipeline completes
no registry reject or generation reject
any inbox overflow is fully accounted as ownerless fallback
final gate opens exactly once before DEAD
alloc/free accounting remains bounded and no object is dropped
```

An inbox-cap overflow is still fail-safe: the rejected batch is immediately
ownerless-freed. L4-B records this as a policy signal rather than treating it
as object loss. An overflow-free run is desirable, but not required for the
diagnostic runner to complete its lifecycle contract.

## Initial Result

On the Windows 1P/1C one-second control, L4-B completed repeat-5 with no
overflow, registry rejection, or generation rejection. Its approximately
10.10M ops/s median was below the owner-id L1 control's approximately 11.66M
ops/s. The eager owner drain deliberately enforces the overflow-free control,
so this result is lifecycle evidence rather than a performance promotion.
