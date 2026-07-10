# HZ12 Windows Retired Inbox Adoption L2-B

## Purpose

L2-B gives a retired owner inbox an explicit successor. It is deliberately
narrow: an adopter may detach the retired owner's already-bounded inbox and
return that chain through the existing ownerless HZ12 free path.

```text
retired owner inbox
  -> detach one bounded chain under that inbox mutex
  -> unlock
  -> adopter frees the chain through the ownerless HZ12 path
```

This is not owner routing on every free, a lock-free queue, or span reclaim.

## Contract

```text
only a marked-retired owner is adoptable
active owner -> adoption rejected without mutation
adoption detaches at most one current bounded inbox chain
the inbox mutex protects detach versus producer publish
objects are released only after unlock through existing ownerless HZ12 free
pointer route validity remains independent of retirement/adoption
an adopted inbox is empty; later publishes still use the normal bounded path
```

The bounded inbox cap limits the amount an adoption call can detach. The
ownerless path is the correctness fallback and does not infer a span's global
free state.

## Required Evidence

```text
active-owner adoption rejection leaves the inbox unchanged
retired-owner adoption detaches exactly the observed pending count
post-adoption shadow snapshot reports zero pending objects
unknown-owner fallback and route safety remain unchanged
normal L1 runner does not call adoption
```

## No-Go

```text
adoption of an active owner succeeds
adoption runs while holding the inbox mutex during hz12_free
adoption changes normal per-free routing
adoption treats empty inbox as proof a span is reclaimable
adoption introduces a per-free owner read or atomic
```

## Next Gate

L2-C may add verified whole-span accounting only after L2-B proves that a
retired inbox can be handed off safely. Span reclaim/decommit stays disabled
until object state, exact routes, and source-span lifetime agree.

## Initial Windows Evidence

The retired-inbox smoke placed eight objects in owner 0's inbox. Adoption while
owner 0 was active was rejected. After the owner was marked retired, one
adoption detached and released all eight objects through the ownerless HZ12
path; the post-adoption snapshot reported zero pending objects. A second
adoption attempt was rejected. The smoke counters recorded one active-owner
rejection, one duplicate rejection, one adoption batch, and eight adoption
objects.

The ordinary 2 producer / 2 consumer L1 xowner run did not call adoption. It
completed with zero unknown-owner and adopted-owner fallback, and its final
retired-owner snapshot was empty. L2-B is therefore a bounded handoff
mechanism pass, not a reclaim or performance promotion.
