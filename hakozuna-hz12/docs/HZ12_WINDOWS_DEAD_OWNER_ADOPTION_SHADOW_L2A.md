# HZ12 Windows Dead-Owner Adoption Shadow L2-A

## Purpose

L2-A introduces lifecycle observation only. It answers whether a retired owner
has a bounded, inspectable inbox that could later be adopted. It does not move
objects, reclaim a span, decommit memory, or change the L1 free path.

```text
L1:
  producer remains alive through the final consumer flush and drains its inbox

L2-A:
  record a retired-owner marker
  snapshot the retired owner's pending inbox objects
  keep ordinary L1 drain behavior unchanged
  verify the snapshot reaches zero after the normal drain
```

## Contract

```text
owner retirement is advisory lifecycle metadata
retired does not make a route invalid
retired does not cause a free to fail or change destination
snapshot reads bounded inbox state under the existing inbox mutex
adoption shadow never detaches, frees, or releases an object
normal owner drain remains the only behavior path in L2-A
```

The later L2-B adoption path may detach a retired inbox only after a separate
dead-owner handoff contract exists. A snapshot alone is never authority to
reclaim a span.

## Required Evidence

```text
retired_owner_marked > 0
retired_owner_with_pending > 0 in the dedicated smoke
adoption_shadow_objects matches the pending inbox count
after normal drain, adoption_shadow_objects = 0
unknown-owner and overflow fallback remain safety-preserving
no HZ11 behavior or source is involved
```

## No-Go

```text
retirement changes ordinary L1 routing or free behavior
snapshot removes an item or changes inbox count
retired owner changes pointer validity
an adoption candidate is treated as a reclaimable span
thread exit can race a detached inbox without an explicit successor contract
```

## Next Gate

If the smoke and normal xowner run retain these invariants, L2-B can introduce
an explicit successor: a retired inbox may be detached by an adopter and moved
to a bounded ownerless sink. Verified whole-span state remains a separate L2-C
gate after that handoff is proven.

## Initial Windows Evidence

The dedicated standalone smoke placed eight HZ12-owned objects in owner 0's
inbox, marked owner 0 retired, and observed `pending_before=8`. It then used
the existing normal owner drain and observed `pending_after=0`. The snapshot
did not detach or free an object.

A 2 producer / 2 consumer one-second xowner run marked both producer owners as
retired. The final read-only snapshot reported `retired_owners=2`,
`retired_pending_owners=0`, and `retired_pending_objects=0`. This is an L2-A
contract pass only. It does not prove dead-owner adoption or span reclaim.
